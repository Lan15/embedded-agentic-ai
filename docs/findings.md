# Pipeline findings: uart_comm (MCU target)

| # | Finding | Found by | Status |
|---|---------|----------|--------|
| 1 | Coder used undeclared header `UART_Driver.h` | Jetson compile check | Fixed: Coder now emits its own `.h` |
| 2 | `UART_STATUS_ERROR_PARAMETER` and `SystemCoreClock` undeclared | Jetson `gcc -fsyntax-only` | Fixed by compile-and-repair loop |
| 3 | `char *` passed as `uint8_t *` (pointer-sign) | Jetson `-Wextra` | Fixed by repair loop |
| 4 | Odd parity set to `0x38` (enables stick parity) instead of `0x08` | Code review of failing test | Fixed manually |
| 5 | Baud divisor truncates: 16 MHz / (16 x 115200) = 8.68 becomes 8, ~8.5% baud error (limit ~3%) | Test review | Open: needs rounding |
| 6 | 5-bit data words rejected although valid | Code review | Open (minor) |
| 7 | Generated tests: swapped Unity arguments, wrong register indexing (segfault), single-register fake cannot model FIFO | Host test run, gdb | Fixed manually; pipeline needs better test prompt |
| 8 | `test_UART_ReceiveString` checks first byte only | Review | Known limit: needs register-access abstraction |

## Process metrics
- Compile-repair loop: used on the first MCU run
- Test-repair loop: failed after 4 rounds (cause: tests, not module), fixed manually
