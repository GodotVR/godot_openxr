@tool
extends EditorPlugin

var openxr_run_select = null
var action_set_editor : OpenXRActionSetsEditor = null
var action_set_button = null

func _enter_tree():
	openxr_run_select = preload("res://addons/godot-openxr/editor/OpenXRRunSelect.tscn").instantiate()
	add_control_to_container(CONTAINER_TOOLBAR, openxr_run_select)
	
	action_set_editor = preload("res://addons/godot-openxr/editor/OpenXRActionSetsEditor.tscn").instantiate()
	if action_set_editor:
		action_set_button = add_control_to_bottom_panel(action_set_editor, "Action Sets")
		action_set_button.hide()

func _exit_tree():
	if action_set_editor:
		remove_control_from_bottom_panel(action_set_editor)
		action_set_editor.queue_free()
		action_set_editor = null
		action_set_button = null

	if openxr_run_select:
		remove_control_from_container(EditorPlugin.CONTAINER_TOOLBAR, openxr_run_select)
		openxr_run_select.queue_free()
		openxr_run_select = null

func _handles(object) -> bool:
	if action_set_editor and object.is_class("OpenXRActionSets"):
		return true
	
	return false

func _edit(object):
	if action_set_editor and object.is_class("OpenXRActionSets"):
		action_set_editor.set_edit_action_sets(object)

func _make_visible(visible):
	if visible:
		action_set_button.show()
		make_bottom_panel_item_visible(action_set_editor)
		action_set_editor.set_process_input(true)
	else:
		if action_set_editor.is_visible_in_tree():
			hide_bottom_panel()
		action_set_button.hide()
		action_set_editor.set_process_input(false)
		
