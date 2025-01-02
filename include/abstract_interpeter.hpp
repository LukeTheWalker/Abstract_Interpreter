// abstract_interpreter.hpp
#ifndef ABSTRACT_INTERPRETER_HPP
#define ABSTRACT_INTERPRETER_HPP

#include "ast.hpp"
#include "interval.hpp"
#include "interval_store.hpp"
#include <memory>
#include <stdexcept>
#include <iostream>

class AbstractInterpreter
{
private:
    using Store = IntervalStore<int64_t>;
    Store store;

    // Helper methods for evaluation
    Interval<int64_t> evalArithmeticExpr(const ASTNode &node);
    bool evalLogicalExpr(const ASTNode &node);
    void evalAssignment(const ASTNode &node);
    void evalIfElse(const ASTNode &node);
    void evalSequence(const ASTNode &node);
    void evalDeclaration(const ASTNode &node);
    void evalPostCondition(const ASTNode &node);
    void checkDivisionByZero(const ASTNode &node);
    void checkOverflow(const Interval<int64_t> &result);

public:
    AbstractInterpreter() = default;

    void eval(const ASTNode &ast)
    {
        try
        {
            // Handle the root node (which is an INTEGER node in the AST)
            if (ast.type == NodeType::INTEGER)
            {
                // Process all child nodes
                for (const auto &child : ast.children)
                {
                    evalNode(child);
                }
            }
            else
            {
                // If not the root node, evaluate normally
                evalNode(ast);
            }
        }
        catch (const std::runtime_error &e)
        {
            std::cerr << "Analysis error: " << e.what() << std::endl;
        }
    }

private:
    void evalNode(const ASTNode &node)
    {
        switch (node.type)
        {
        case NodeType::DECLARATION:
            evalDeclaration(node);
            break;
        case NodeType::SEQUENCE:
            evalSequence(node);
            break;
        case NodeType::ASSIGNMENT:
            evalAssignment(node);
            break;
        case NodeType::IFELSE:
            evalIfElse(node);
            break;
        case NodeType::POST_CON:
            evalPostCondition(node);
            break;
        default:
            std::cout << "Unhandled node type: " << static_cast<int>(node.type) << std::endl;
            break;
        }
    }
};

// Implement arithmetic expression evaluation
Interval<int64_t> AbstractInterpreter::evalArithmeticExpr(const ASTNode &node)
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
        auto left = evalArithmeticExpr(node.children[0]);
        auto right = evalArithmeticExpr(node.children[1]);
        BinOp op = std::get<BinOp>(node.value);

        // Check for division by zero
        if (op == BinOp::DIV)
        {
            checkDivisionByZero(node);
        }

        Interval<int64_t> result;
        switch(op)
        {
        case BinOp::ADD:
            result = left + right;
            break;
        case BinOp::SUB:
            result = left + (-right);
            break;
        default:
            std::cerr << "Unsupported arithmetic operation" << std::endl;
            return Interval<int64_t>();
        }
        checkOverflow(result);
        return result;
    }
    throw std::runtime_error("Invalid arithmetic expression");
}

// Implement logical expression evaluation
bool AbstractInterpreter::evalLogicalExpr(const ASTNode &node)
{
    if (node.type != NodeType::LOGIC_OP)
    {
        throw std::runtime_error("Expected logical operation");
    }

    auto left = evalArithmeticExpr(node.children[0]);
    auto right = evalArithmeticExpr(node.children[1]);
    LogicOp op = std::get<LogicOp>(node.value);

    switch (op)
    {
    case LogicOp::EQ:
        return left == right;
    case LogicOp::NEQ:
        return left != right;
    case LogicOp::LE:
        return left < right;
    case LogicOp::LEQ:
        return left <= right;
    case LogicOp::GE:
        return left > right;
    case LogicOp::GEQ:
        return left >= right;
    }
    return false;
}

// Handle declarations
void AbstractInterpreter::evalDeclaration(const ASTNode &node)
{
    if (node.children.size() != 1)
    {
        throw std::runtime_error("Invalid declaration");
    }

    std::string var = std::get<std::string>(node.children[0].value);
    // Initialize with top interval
    store.update_interval(var, Interval<int64_t>());
}

// Implement assignment evaluation
void AbstractInterpreter::evalAssignment(const ASTNode &node)
{
    if (node.children.size() != 2)
    {
        throw std::runtime_error("Invalid assignment");
    }

    std::string var = std::get<std::string>(node.children[0].value);
    Interval<int64_t> value = evalArithmeticExpr(node.children[1]);
    store.update_interval(var, value);
}

// Implement if-else evaluation
void AbstractInterpreter::evalIfElse(const ASTNode &node)
{
    if (node.children.size() != 3)
    {
        throw std::runtime_error("Invalid if-else statement");
    }

    Store true_branch = store;
    Store false_branch = store;

    // Evaluate condition
    bool condition = evalLogicalExpr(node.children[0]);

    // Evaluate true branch
    store = true_branch;
    evalNode(node.children[1]);
    Store after_true = store;

    // Evaluate false branch
    store = false_branch;
    evalNode(node.children[2]);
    Store after_false = store;

    // Join the results
    store = after_true.join(after_false);
}

// Implement sequence evaluation
void AbstractInterpreter::evalSequence(const ASTNode &node)
{
    for (const auto &child : node.children)
    {
        evalNode(child);
    }
}

// Implement postcondition (assertion) evaluation
void AbstractInterpreter::evalPostCondition(const ASTNode &node)
{
    if (node.children.empty())
    {
        throw std::runtime_error("Invalid assertion");
    }

    if (!evalLogicalExpr(node.children[0]))
    {
        std::cerr << "Assertion might fail: " << std::endl;
        // Print the current store state for debugging
        store.print();
    }
    else
    {
        std::cout << "Assertion verified successfully" << std::endl;
    }
}

// Check for division by zero
void AbstractInterpreter::checkDivisionByZero(const ASTNode &node)
{
    auto divisor = evalArithmeticExpr(node.children[1]);
    if (divisor.contains(0))
    {
        std::cerr << "Warning: Possible division by zero detected!" << std::endl;
    }
}

// Check for integer overflow
void AbstractInterpreter::checkOverflow(const Interval<int64_t> &result)
{
    if (result.getLower() == std::numeric_limits<int64_t>::lowest() ||
        result.getUpper() == std::numeric_limits<int64_t>::max())
    {
        std::cerr << "Warning: Possible integer overflow detected!" << std::endl;
    }
}

#endif