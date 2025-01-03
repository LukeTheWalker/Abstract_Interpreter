int a;
int b;
int c;
int d;

void main() {
  c = 5;
  d = 0;
  c = c / d;
  /*!npk a between 2 and 4 */
  b = (a * a) / a;
  // Note the loss of precision with interval analysis.
  assert(b >= 1);
  assert(b <= 8);
}
