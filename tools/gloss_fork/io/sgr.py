# Copyright 2010 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.

'''
ANSI Select Graphics Rendition (SGR) sequences.

Abbreviations:

R: reset

B: bold
U: underline
N: blink
I: invert
T: color text
G: color background

D: dark gray
R: red
G: green
Y: yellow
B: blue
M: magenta
C: cyan
L: light gray
'''

import gloss_fork.io.cs as _cs


def sgr(*seq):
  'Select Graphic Rendition control sequence string.'
  return _cs.cs('m', *seq)


# TODO: consider making these strings if it improves performance.

# reset: all, bold, underline, blink, invert, color text, color background
R, RB, RU, RN, RI, RT, RG = [0, 22, 24, 25, 27, 39, 49]

# effects: bold, underline, blink, invert
B, U, N, I = [1, 4, 5, 7]

# color text: dark gray, red, green, yellow, blue, magenta, cyan, light gray
TD, TR, TG, TY, TB, TM, TC, TL = text_primary = list(range(30, 38))

# color background: dark gray, red, green, yellow, blue, magenta, cyan, light gray
GD, GR, GG, GY, GB, GM, GC, GL = background_primary = list(range(40, 48))

# xterm-256 sequence initiators; these should be followed by a single color index.
# both text and background can be specified in a single sgr call.
T = '38;5'
G = '48;5'

# 256-indexed color palettes

K = 16  # black
W = 231 # white
# the rgb palette is a 6x6x6 color cube, lying in between black and white.
colors = list(range(K, W + 1))

# grayscale
# the 24 grayscale palette values have a suggested 8 bit range of [8, 238].
# for convenience, these are given symbolic names.
K1, K2, K3, K4, K5, K6, K7, K8, K9, KA, KB, KC, \
WC, WB, WA, W9, W8, W7, W6, W5, W4, W3, W2, W1, \
= _grays = list(range(232, 256))

# absolute black and white are obtained from the low and high values on the color cube.
grays = [K] + _grays + [W]

# colors

def rgb_colors_index(r, g, b):
  'index RGB triples into the 256-color palette color cube (returns 0 for black, 215 for white).'
  assert 0 <= r < 6
  assert 0 <= g < 6
  assert 0 <= b < 6
  return (((r * 6) + g) * 6) + b


def rgb(r, g, b):
  'index RGB triples into the 256-color palette (returns 16 for black, 231 for white).'
  assert 0 <= r < 6
  assert 0 <= g < 6
  assert 0 <= b < 6
  return (((r * 6) + g) * 6) + b + 16


def w(i):
  'index into the grayscale palette.'
  return grays[i]


# palettes

reds    = [rgb(i, 0, 0) for i in range(6)]
greens  = [rgb(0, i, 0) for i in range(6)]
blues   = [rgb(0, 0, i) for i in range(6)]

blue_cyans = blues + [rgb(0, i, 5) for i in range(1, 6)]
blue_cyan_greens = blue_cyans + [rgb(0, 5, i) for i in range(4, -1, -1)]

heat = \
[rgb(i, 0, 0) for i in range(5)] + \
[rgb(5, i, 0) for i in range(5)] + \
[rgb(5, 5, i) for i in range(6)]

cool = \
[rgb(0, 0, i) for i in range(5)] + \
[rgb(0, i, 5) for i in range(5)] + \
[rgb(i, 5, 5) for i in range(6)]


def normal(palette, value):
  'given a palette and normalized color float value return a palette value.'
  assert 0.0 <= value <= 1.0
  i = int(round(value * (len(palette) - 1)))

  try:
    return palette[i]
  except IndexError as e:
    e.args = ('value: {}; index: {}; len(palette): {}'.format(value, i, len(palette)))
    raise e


char_codes_to_color_squares = { ord(t[0]) : sgr(*t[1:]) + '  ' for t in [
  ('k', G, K),
  ('d', GD),
  ('r', GR),
  ('g', GG),
  ('y', GY),
  ('b', GB),
  ('m', GM),
  ('c', GC),
  ('l', GL),
  ('w', G, W),
]}

char_codes_to_color_squares[ord('\n')] = sgr(RG) + '\n'


if __name__ == '__main__':

  def p(sgr_items, desc):
    'print a bullet for each sgr string; surround the items with dashes for debugging visibility.'
    l = list(i + '•' for i in sgr_items)
    l.insert(0, '-')
    l.extend([sgr(R), '- ', desc])
    print(*l, sep='')

  def tg(colors, desc):
    'print a sequence of colors (both text and background).'
    p((sgr(T, c, G, c) for c in colors), desc)

  def steps(palette):
    'generate normalized values to iterate exactly over the given palette.'
    interval = 1 / (len(palette) - 1)
    x = 0.0
    while x < 1.0:
      yield x
      x += interval
    yield 1.0


  p((sgr(text_primary[i]) for i in range(8)), 'traditional named colors')
  p((sgr(text_primary[i], background_primary[i]) for i in range(8)), 'text may be lighter than background')
  tg(range(0, 8), 'first 8 256-indexed colors should match named colors')
  tg(range(8, 16), 'next 8 256-indexed colors are bright')

  print()
  tg(grays, 'grays')
  tg((normal(grays, f) for f in steps(grays)), 'grays (normalized input)')

  print()
  tg(reds, 'reds')
  tg(greens, 'greens')
  tg(blues, 'blues')

  print()
  tg(blue_cyans, 'blue_cyans')
  tg(blue_cyan_greens, 'blue_cyan_greens')
  tg(cool, 'cool')
  tg(heat, 'heat')
  p((sgr(T, normal(cool, f), G, normal(heat, f)) for f in steps(cool)), 'cool on heat (normalized input)')
  
  print()
  for r in range(6):
    for g in range(6):
      for b in range(6):
        print(sgr(T, rgb(r, g, b), G, rgb(r, g, b)), '•', sep='', end='')
      print(sgr(R), ' ', sep='', end='')
    print()

  art = '''
kwrgbwk
wkcmykw
'''
  print(art.translate(char_codes_to_color_squares))

  # terminal emulators have weird behavior regarding newline and color.
  # this prints differently depending on whether the terminal has scrolled or not.
  bug = sgr(GR) + 'red-background\n' + sgr(R) + 'normal\nnormal\n'
  print('osx terminal bug (manifests once scrolling begins):', repr(bug))
  print(bug)

  # suggested workaround
  #'\x1b[41mred-background\x1b[K\x1b[0m\nnormal\nnormal\n'
  bug = sgr(GR) + 'red-background\nmore' + _cs.ERASE_LINE_F + sgr(RG) + '\nnormal\nnormal\n'
  print('suggested workaround:', repr(bug))
  print(bug)

  #italic = sgr()


