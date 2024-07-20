"""
Since we're reinventing the wheel, testing expressions is easy!

If a.out puts debuginfo to stderr, then call

./a.out 2>&1 1>/dev/null | python3 check_expr.py
"""
import sys

def check(expr, idx):
    if '#' in expr:
        expr = expr.split('#')[0]

    expr = expr.strip()
    if not expr:
        return True

    try:
        if not eval(expr):
            print(f"{idx+1: 5d}:", expr, "is false!", file=sys.stderr)
            return False
        return True
    except Exception as e:
        print("Encountered", type(e).__name__, f"[[ {e} ]]", "when evaluating", expr)
        return False


if __name__ == "__main__":
    all_pass = True
    for idx, expr in enumerate(sys.stdin):
        all_pass &= check(expr, idx)
    if all_pass:
        print("All checks passed!")
