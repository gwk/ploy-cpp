{View} = require 'atom'

module.exports =
class PloyAtomView extends View
  @content: ->
    @div class: 'ploy-atom overlay from-top', =>
      @div "This is the dummy action from the generated package (left for future reference)", class: "message"

  initialize: (serializeState) ->
    atom.workspaceView.command "ploy-atom:dummy", => @dummy()

  # Returns an object that can be retrieved when package is activated
  serialize: ->

  # Tear down any state and detach
  destroy: ->
    @detach()

  dummy: ->
    console.log "PloyAtomView dummy action!"
    if @hasParent()
      @detach()
    else
      atom.workspaceView.append(this)
