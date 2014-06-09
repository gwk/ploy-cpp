PloyAtomView = require './ploy-atom-view'

module.exports =
  ployAtomView: null

  activate: (state) ->
    @ployAtomView = new PloyAtomView(state.ployAtomViewState)

  deactivate: ->
    @ployAtomView.destroy()

  serialize: ->
    ployAtomViewState: @ployAtomView.serialize()
