// abstract_interpreter.hpp
#ifndef ABSTRACT_INTERPRETER_HPP
#define ABSTRACT_INTERPRETER_HPP

#include "ast.hpp"
#include "interval.hpp"
#include "interval_store.hpp"
#include <memory>
#include <stdexcept>
#include <iostream>
#include <functional>

using Store = IntervalStore<int64_t>;

LogicOp negate_logic_op(LogicOp op)
{
    switch (op)
    {
    case LogicOp::EQ:
        return LogicOp::NEQ;
    case LogicOp::NEQ:
        return LogicOp::EQ;
    case LogicOp::LE:
        return LogicOp::GEQ;
    case LogicOp::LEQ:
        return LogicOp::GE;
    case LogicOp::GE:
        return LogicOp::LEQ;
    case LogicOp::GEQ:
        return LogicOp::LE;
    default:
        std::cerr << "Unsupported logical operation" << std::endl;
        return LogicOp::EQ;
    }
}

Interval<int64_t> evalArithmeticExpr(const ASTNode &node, const Store& store)
{
    if (node.type == NodeType::INTEGER)
    {
        int64_t value = std::get<int>(node.value);
        return Interval<int64_t>(value, value);
    }
    else if (node.type == NodeType::VARIABLE)
    {
        std::string var = std::get<std::string>(node.value);
        return store.get_interval(var);
    }
    else if (node.type == NodeType::ARITHM_OP)
    {
        auto left = evalArithmeticExpr(node.children[0], store);
        auto right = evalArithmeticExpr(node.children[1], store);
        BinOp op;
        try {
            op = std::get<BinOp>(node.value);
        } catch (const std::bad_variant_access&) {
            std::string op_str = std::get<std::string>(node.value);
            op = op_str == "+" ? BinOp::ADD : 
                 op_str == "-" ? BinOp::SUB : 
                 op_str == "*" ? BinOp::MUL : 
                 op_str == "/" ? BinOp::DIV : BinOp::ADD;
        }

        Interval<int64_t> result;
        switch(op)
        {
            case BinOp::ADD:
            {
                // Simple overflow check for intervals (clamp seen as a conservative approximation):
                if ((left.getLower() <= std::numeric_limits<int32_t>::lowest() && right.getLower() < 0) ||
                    (left.getUpper() >= std::numeric_limits<int32_t>::max() && right.getUpper() > 0)) {
                    std::cerr << "Warning: potential ADD overflow detected, clamping." << std::endl;
                }
                result = left + right;
                break;
            }
            case BinOp::SUB:
            {
                // Similar check for SUB:
                if ((left.getLower() <= std::numeric_limits<int32_t>::lowest() && right.getUpper() > 0) ||
                    (left.getUpper() >= std::numeric_limits<int32_t>::max() && right.getLower() < 0)) {
                    std::cerr << "Warning: potential SUB overflow detected, clamping." << std::endl;
                }
                result = left - right;
                break;
            }
            case BinOp::MUL:
            {
                // Check if either interval spans large values that could overflow:
                if ((std::abs(left.getLower()) >= std::numeric_limits<int32_t>::max() &&
                    std::abs(right.getLower()) > 1) ||
                    (std::abs(left.getUpper()) >= std::numeric_limits<int32_t>::max() &&
                    std::abs(right.getUpper()) > 1)) {
                    std::cerr << "Warning: potential MUL overflow detected, clamping." << std::endl;
                }
                result = left * right;
                break;
            }
            case BinOp::DIV:
            {
                // Division by zero check:
                if (right.contains(0)) {
                    std::cerr << "Warning: division by zero detected, clamping result to full range." << std::endl;
                    result = Interval<int64_t>(
                        std::numeric_limits<int64_t>::lowest(),
                        std::numeric_limits<int64_t>::max()
                    );
                }
                else {
                    result = left / right;
                }
                break;
            }
            default:
                std::cerr << "Unsupported arithmetic operation" << std::endl;
                return Interval<int64_t>();
        }
        return result;
    }
    else
    {
        throw std::runtime_error("Unsupported node type");
    }   
}


Interval<int64_t> evalLogicalExpr(const ASTNode &node, const Store& store)
{
    if (node.type != NodeType::LOGIC_OP)
    {
        throw std::runtime_error("Expected logical operation");
    }

    auto left = evalArithmeticExpr(node.children[0], store);
    auto right = evalArithmeticExpr(node.children[1], store);
    LogicOp op = std::get<LogicOp>(node.value);

    int32_t left_lower = static_cast<int32_t>(left.getLower() == std::numeric_limits<int64_t>::lowest() ? std::numeric_limits<int32_t>::lowest() : left.getLower());
    int32_t right_lower = static_cast<int32_t>(right.getLower() == std::numeric_limits<int64_t>::lowest() ? std::numeric_limits<int32_t>::lowest() : right.getLower());
    int32_t left_upper = static_cast<int32_t>(left.getUpper() == std::numeric_limits<int64_t>::max() ? std::numeric_limits<int32_t>::max() : left.getUpper());
    int32_t right_upper = static_cast<int32_t>(right.getUpper() == std::numeric_limits<int64_t>::max() ? std::numeric_limits<int32_t>::max() : right.getUpper());

    Interval<int64_t> result;

    // print the intervals
    std::cout << "Left: [" << left_lower << ", " << left_upper << "]" << std::endl;
    std::cout << "Right: [" << right_lower << ", " << right_upper << "]" << std::endl;

    switch (op)
    {
    case LogicOp::EQ:
        // Return the intersection of the intervals
        result = Interval<int64_t>(
            std::max(left_lower, right_lower),
            std::min(left_upper, right_upper)
        );
        break;

    case LogicOp::NEQ:
        {
            Interval<int64_t> intersection(
                std::max(left_lower, right_lower),
                std::min(left_upper, right_upper)
            );

            // If the intervals overlap by exactly one value and that value equals
            // the entire left interval, return empty; otherwise keep the left interval.
            if (intersection.getLower() == intersection.getUpper() &&
                left.getLower() == left.getUpper() &&
                intersection.getLower() == left.getLower())
            {
                result = Interval<int64_t>::build_empty();
            }
            else
            {
                // If there's more than one point to exclude (would split the interval),
                // or no overlap, just keep the original left interval as an approximation.
                result = left;
            }
        }
        break;

    case LogicOp::LE:
        // x < y: Return all x values that are less than the maximum y
        result = Interval<int64_t>(
            left_lower,
            left_upper < right_upper ? left_upper : right_upper - 1
        );
        break;

    case LogicOp::LEQ:
        // x <= y: Return all x values that are less than or equal to the maximum y
        result = Interval<int64_t>(
            left_lower,
            left_upper < right_upper ? left_upper : right_upper
        );
        break;

    case LogicOp::GE:
        // x > y: Return all x values that are greater than the minimum y
        result = Interval<int64_t>(
            left_lower > right_lower ? left_lower : right_lower + 1,
            left_upper
        );
        break;

    case LogicOp::GEQ:
        // x >= y: Return all x values that are greater than or equal to the minimum y
        result = Interval<int64_t>(
            left_lower > right_lower ? left_lower : right_lower,
            left_upper
        );
        break;

    default:
        throw std::runtime_error("Unsupported logical operation");
    }

    return result;
}

class location {
public:
    Store store;
    std::vector<const Store*> deps;
    location(const Store &store, const std::vector<const Store*> &deps) : store(store), deps(deps) {}
    virtual bool eval() = 0;
    virtual ~location() = default;
};

class declaration_location : public location {
public:
    declaration_location(const Store &store, const std::vector<const Store*> &deps) : location(store, deps) {}
    bool eval() override { std::cout << "Evaluating declaration" << std::endl; return true; }
};

class assignment_location : public location {
    const ASTNode& node;
public:
    assignment_location(const ASTNode& node, const Store &store, const std::vector<const Store*> &deps)
        : location(store, deps), node(node) {}

    bool eval() override { 
        std::string var = std::get<std::string>(node.children[0].value);
        Interval<int64_t> value = evalArithmeticExpr(node.children[1], *(deps[0]));
        std::cout << "Evaluating assignment: " << var << " = [" << value.getLower() << ", " << value.getUpper() << "]" << std::endl;
        auto new_store = assignment_eq(var, value);
        bool changed = (store == new_store);
        store = new_store;
        return changed; 
    }

    Store assignment_eq (const std::string &var, const Interval<int64_t> &value) {
        Store new_store = *(deps[0]);
        new_store.update_interval(var, value);
        return new_store;
    }
};

class precondition_location : public location {
    const ASTNode &node;
public:
    precondition_location(const ASTNode &node, const Store &store, const std::vector<const Store*> &deps)
        : location(store, deps), node(node) {}

    bool eval() override {
        Store new_store = *(deps[0]);
        if (node.children.size() != 2) {
            throw std::runtime_error("Invalid precondition");
        }
        std::string var = std::get<std::string>(node.children[0].children[1].value);
        int64_t lb = std::get<int>(node.children[0].children[0].value);
        int64_t ub = std::get<int>(node.children[1].children[0].value);
        new_store.update_interval(var, Interval<int64_t>(lb, ub));
        bool changed = (store == new_store);
        store = new_store;
        return changed;
    }
};

class preif_location : public location {
    const ASTNode &node;
    const std::string var;
    const ASTNode logic_node;
public:
    preif_location(const ASTNode &logic_node, const std::string &var, const ASTNode &node, const Store &store, const std::vector<const Store*> &deps) 
        : location(store, deps), logic_node(logic_node), var(var), node(node) {}

    bool eval() override {
        Store new_store = *(deps[0]);

        // new_store.update_interval(var, evalLogicalExpr(logic_node, new_store)); 
        new_store.update_interval(var, evalLogicalExpr(logic_node, new_store).meet(new_store.get_interval(var)));

        bool changed = (store == new_store);
        store = new_store;
        return changed;
    }
};

class ifelse_location : public location {
std::shared_ptr<location> iflocation;
std::shared_ptr<location> elselocation;
public:
    ifelse_location (std::shared_ptr<location>& iflocation, std::shared_ptr<location>& elselocation, const Store &store, const std::vector<const Store*> &deps) : location(store, deps), iflocation(iflocation), elselocation(elselocation) {}
    bool eval() {
        Store new_store = iflocation->store.join(elselocation->store);
        bool changed = (store == new_store);
        store = new_store;
        return changed;
    }
};

class prewhile_location : public location {
    const ASTNode &node;
    const std::string var;
    const ASTNode logic_node;
    bool first = true;
public:
    Store *postwhile_store;

    prewhile_location(const ASTNode &logic_node, const std::string &var, const ASTNode &node, const Store &store, const std::vector<const Store*> &deps)
        : location(store, deps), logic_node(logic_node), var(var), node(node) {}
    bool eval() override {
        Store new_store = *(deps[0]);

        if (first) first = false;
        else new_store = new_store.join(*postwhile_store);

        // Widening
        {
            Interval<int64_t> old_iv = store.get_interval(var);
            Interval<int64_t> joined_iv = new_store.get_interval(var);
            int64_t widened_lower = (old_iv.getLower() > joined_iv.getLower()) ? std::numeric_limits<int64_t>::lowest() : old_iv.getLower();
            int64_t widened_upper = (old_iv.getUpper() < joined_iv.getUpper()) ? std::numeric_limits<int64_t>::max() : old_iv.getUpper();
            new_store.update_interval(var, Interval<int64_t>(widened_lower, widened_upper));
        }

        new_store.update_interval(
            var,
            evalLogicalExpr(logic_node, new_store).meet(new_store.get_interval(var))
        );

        bool changed = (store == new_store);
        store = new_store;
        return changed;
        }

};

class postwhile_location : public location {
    const ASTNode &node;
    const std::string var;
    ASTNode logic_node;
public:

    postwhile_location(const ASTNode &logic_node, const std::string &var, const ASTNode &node, const Store &store, const std::vector<const Store*> &deps)
        : location(store, deps), logic_node(logic_node), var(var), node(node) {
            // negate the logic node
            this->logic_node.value = negate_logic_op(std::get<LogicOp>(logic_node.value));
        }

    bool eval() override {
        Store new_store = *(deps[0]);

        std::cout << "Logical expression: " << std::get<LogicOp>(logic_node.value) << std::endl;

        std::cout << "prestore: " << std::endl;
        new_store.print();

        new_store.update_interval(var, evalLogicalExpr(logic_node, new_store).meet(new_store.get_interval(var)));

        std::cout << "poststore: " << std::endl;
        new_store.print();

        bool changed = (store == new_store);
        store = new_store;
        return changed;
    }
};


class AbstractInterpreter
{
private:
    using Store = IntervalStore<int64_t>;
    std::vector<std::shared_ptr<location>> locations;
    bool end = false;
    uint32_t iteration = 0;

public:
    AbstractInterpreter() = default;

    void create_top_locations(const ASTNode& ast) {
        locations.push_back(std::make_shared<declaration_location>(Store(), std::vector<const Store*>{}));
        for (const auto& top_level_child : ast.children) {
            if (top_level_child.type == NodeType::DECLARATION) {
                for (const auto& child : top_level_child.children) {
                    std::string var = std::get<std::string>(child.value);
                    locations[0]->store.update_interval(var, Interval<int64_t>());
                }
            }
            else if (top_level_child.type == NodeType::SEQUENCE)
                for (const auto& child : top_level_child.children) create_locations(child, locations.size() - 1);
            }
        }


    void create_locations(const ASTNode& ast, size_t i) {
        if (ast.type == NodeType::ASSIGNMENT) {
            locations.push_back(std::make_shared<assignment_location>(
                ast,
                locations[i]->store, 
                std::vector<const Store*>{&(locations[i]->store)}
            ));
        }
        else if (ast.type == NodeType::PRE_CON) {
            locations.push_back(std::make_shared<precondition_location>(
                ast, 
                locations[i]->store, 
                std::vector<const Store*>{&(locations[i]->store)}
            ));
        }
        else if (ast.type == NodeType::IFELSE) {
            Store if_store = locations[i]->store;

            auto logic_node = ast.children[0].children[0];

            auto variable_node = logic_node.children[0];

            locations.push_back(std::make_shared<preif_location>(logic_node, std::get<std::string>(variable_node.value), ast.children[1].children[0], if_store, std::vector<const Store*>{&(locations[i]->store)}));
            create_locations(ast.children[1].children[0], locations.size() - 1);

            auto iflocation = locations.back();

            Store else_store = locations[i]->store;

            if (ast.children.size() == 3) 
                logic_node.value = negate_logic_op(std::get<LogicOp> (logic_node.value));
            
            if (ast.children.size() == 3)
                locations.push_back(std::make_shared<preif_location>(logic_node, std::get<std::string>(variable_node.value), ast.children[2].children[0], else_store, std::vector<const Store*>{&(locations[i]->store)}));

            if (ast.children.size() == 3) 
                create_locations(ast.children[2].children[0], locations.size() - 1);

            auto elselocation = locations.back();

            locations.push_back(std::make_shared<ifelse_location>(iflocation, elselocation, locations[i]->store, std::vector<const Store*>{}));

        }
        else if (ast.type == NodeType::WHILELOOP){
            Store while_store = locations[i]->store;
            auto logic_node = ast.children[0].children[0];
            auto variable_node = logic_node.children[0];
            locations.push_back(std::make_shared<prewhile_location>(logic_node, std::get<std::string>(variable_node.value), ast.children[1].children[0], while_store, std::vector<const Store*>{&(locations[i]->store)}));
            auto whilelocation = locations.back();
            create_locations(ast.children[1].children[0], locations.size() - 1);
            auto postwhile_store = locations.back();
            std::dynamic_pointer_cast<prewhile_location>(whilelocation)->postwhile_store = &(postwhile_store->store);
            locations.push_back(std::make_shared<postwhile_location>(logic_node, std::get<std::string>(variable_node.value), ast.children[1].children[0], while_store, std::vector<const Store*>{&(locations.back()->store)}));

        }
        else if (ast.type == NodeType::SEQUENCE) for (const auto& child : ast.children) create_locations(child, locations.size() - 1);
        else if (ast.type == NodeType::POST_CON) std::cout << "Post condition found" << std::endl;
        else { std::cerr << "Unsupported node type" << ": " << ast.type << std::endl; std::cout << "Skipping..." << std::endl; ast.print(); }
    }

    void eval_all(){
        while (!end){
            std::cin.get();
            std::cout << "Iteration " << iteration << std::endl;
            end = true;
            for (size_t i = 0; i < locations.size(); ++i) {
                std::cout << "Evaluating location " << i << "..." << std::endl;
                auto &loc = locations[i];
                end = loc->eval() && end;
                loc->store.print();
            }
            iteration++;
        }
        std::cout << "Fixed point reached after " << iteration - 1 << " iterations" << std::endl;
    }

    void check_assertions(const ASTNode& ast){
        if (locations.empty()){ std::cerr << "No locations to check assertions" << std::endl; return; }
        Store store = locations.back()->store;
        const auto &seq = ast.children.back();
        for (const auto &child : seq.children){
            if (child.type == NodeType::POST_CON){
                const auto& assertion_interval = evalLogicalExpr(child.children[0], store);
                if (assertion_interval.getLower() > assertion_interval.getUpper()){
                    std::cerr << "Assertion might fail: " << std::endl;
                    child.children[0].print();
                    std::cout << "Current store state:" << std::endl;
                    store.print();
                }
                else
                {
                    std::cout << "Assertion verified successfully" << std::endl;
                }
            }
        }
        std::cout << "Final store state:" << std::endl;
        store.print();
    }
};

#endif