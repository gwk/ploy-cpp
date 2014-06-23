# Copyright 2013 George King.
# Permission to use this file is granted in ploy/license.txt.

# forked from gloss python libs.
# TODO: remove these dependencies.


class Marker:

  def __init__(self, desc):
    self.description = desc

  def __str__(self):
    return '<Marker: {}>'.format(self.description)

  def __repr__(self):
    return '<Marker {}: {}>'.format(hex(id(self)), self.description)
