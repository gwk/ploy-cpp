{WorkspaceView} = require 'atom'
PloyAtom = require '../lib/ploy-atom'

# Use the command `window:run-package-specs` (cmd-alt-ctrl-p) to run specs.
#
# To run a specific `it` or `describe` block add an `f` to the front,
# (e.g. `fit` or `fdescribe`). Remove the `f` to unfocus the block.

describe "PloyAtom", ->
  activationPromise = null

  beforeEach ->
    atom.workspaceView = new WorkspaceView
    activationPromise = atom.packages.activatePackage('ploy-atom')

  describe "when the ploy-atom:dummy event is triggered", ->
    it "attaches and then detaches the view", ->
      expect(atom.workspaceView.find('.ploy-atom')).not.toExist()

      # This is an activation event, triggering it will cause the package to be activated.
      atom.workspaceView.trigger 'ploy-atom:dummy'

      waitsForPromise ->
        activationPromise

      runs ->
        expect(atom.workspaceView.find('.ploy-atom')).toExist()
        atom.workspaceView.trigger 'ploy-atom:dummy'
        expect(atom.workspaceView.find('.ploy-atom')).not.toExist()
