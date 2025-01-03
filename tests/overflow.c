int a;

void main() {
  a = 2147483647;
  a = a + 2;
  assert(a == -2147483647);
}
