#!/usr/bin/env python3

PROP_LETTER    = 1 << 0
PROP_DIGIT     = 1 << 1
PROP_HEX       = 1 << 2
PROP_USER      = 1 << 3
PROP_SPECIAL   = 1 << 4
PROP_PREFIX    = 1 << 5

def letter_mask(n):
    if ord('a') <= n <= ord('z'):
        return PROP_LETTER
    elif ord('A') <= n <= ord('Z'):
        return PROP_LETTER
    else:
        return 0

def digit_mask(n):
    return PROP_DIGIT if ord('0') <= n <= ord('9') else 0

def hex_mask(n):
    if ord('a') <= n <= ord('f') or ord('A') <= n <= ord('F'):
        return PROP_HEX
    elif ord('0') <= n <= ord('9'):
        return PROP_HEX
    else:
        return 0

def username_mask(n):
    non_username_chars = set(ord(c) for c in ['\0', '\r', '\n', ' ', '@'])
    return PROP_USER if n not in non_username_chars else 0

def special_mask(n):
    if (n >= 0x5B and n <= 0x60) or (n >= 0x7B and n <= 0x7D):
        return PROP_SPECIAL
    else:
        return 0

def prefix_mask(n):
    if letter_mask(n) != 0:
        return PROP_PREFIX
    elif digit_mask(n) != 0:
        return PROP_PREFIX
    elif special_mask(n) != 0:
        return PROP_PREFIX
    elif n in set(ord(c) for c in ['!', '.', '@', '-', ':']):
        return PROP_PREFIX
    else:
        return 0

def build_table():
    table = [
        letter_mask(i) |
        digit_mask(i) |
        hex_mask(i) |
        username_mask(i) |
        special_mask(i) |
        prefix_mask(i)

        for i in range(0, 256)
    ]

    print('{')
    for i in range(0, len(table)):
        line = i // 16
        col = i % 16

        if col == 0:
            print('  ', end='')
        
        if col == 15:
            sep = ',\n'
        else:
            sep = ', '
        
        print('{0:#04x}'.format(table[i]), end=sep)
    print('}')


if __name__ == '__main__':
    build_table()
