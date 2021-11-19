@tool
extends HBoxContainer

var action = OpenXRAction

signal remove_action(action)

# missing the UI for setting these, but since this is a tool script we can run it once and voila
#func _ready():
#	$ActionType.clear()
#	$ActionType.add_item("Bool", 0)
#	$ActionType.add_item("Float", 1)
#	$ActionType.add_item("Vector2", 2)
#	$ActionType.add_item("Pose", 3)

func set_action(p_action : OpenXRAction):
	if (action == p_action):
		print("Unchanged action")
		return

	action = p_action

	_update_action()

func _update_action():
	if action:
		$Name.text = action.name
		$ActionType.selected = action.action_type

func _on_DeleteActionBtn_pressed():
	if action:
		emit_signal("remove_action", action)

func _on_Name_text_changed():
	if action:
		action.name = $Name.text

func _on_ActionType_item_selected(index):
	if action:
		action.action_type = index
