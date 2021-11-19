@tool
extends VBoxContainer

@onready var expanded_node : Button = $Header/Expanded
@onready var name_node : TextEdit = $Header/Name
@onready var priority_node : TextEdit = $Header/Priority

@onready var expanded_icon = preload("res://addons/godot-openxr/editor/icons/GuiTreeArrowDown.svg")
@onready var collapsed_icon = preload("res://addons/godot-openxr/editor/icons/GuiTreeArrowRight.svg")
@onready var action_scene = preload("res://addons/godot-openxr/editor/OpenXRActionEditor.tscn")

var action_set : OpenXRActionSet
var is_expanded = true

signal remove_action_set(action_set)

func _ready():
	_update_expanded()

func set_action_set(p_action_set : OpenXRActionSet):
	if (action_set == p_action_set):
		# unchanged
		return

	if action_set:
		# remove signals
		action_set.disconnect("actions_changed", _update_action_set)

	action_set = p_action_set

	if action_set:
		# add signals
		action_set.connect("actions_changed", _update_action_set)

	_update_action_set();

func _update_expanded():
	if is_expanded:
		expanded_node.icon = expanded_icon
	else:
		expanded_node.icon = collapsed_icon
	
	for node in get_children():
		if node.name == "Header":
			node.visible = true
		elif node.name == "Spacer":
			node.visible = not(is_expanded)
		else:
			node.visible = is_expanded

func _update_action_set():
	if action_set:
		name_node.text = action_set.name
		priority_node.text = str(action_set.priority)

	var current_actions = get_children()

	if action_set:
		var i = 1
		for action in action_set.get_actions():
			var node_name = "action_" + str(i)
			var node = get_node_or_null(node_name)
			if node:
				# remove it from our current_action_sets, we are reusing this
				current_actions.erase(node)
			else:

				# create a new node
				node = action_scene.instantiate()
				node.name = node_name
				node.visible = is_expanded
				add_child(node)
				node.connect("remove_action", _on_RemoveAction)

			node.set_action(action)

			i = i + 1

	# remove the remainders, we don't need them anymore...
	for node in current_actions:
		if str(node.name).begins_with("action_"):
			remove_child(node)
			node.queue_free()

func _on_RemoveAction(p_action : OpenXRAction):
	if action_set:
		action_set.remove_action(p_action)

func _on_Name_text_changed():
	if action_set:
		action_set.name = name_node.text

func _on_RemActionSetBtn_pressed():
	if action_set:
		emit_signal("remove_action_set", action_set)

func _on_NewActionBtn_pressed():
	if action_set:
		var action = OpenXRAction.new()
		action.name = "new"
		action_set.add_action(action)

func _on_Priority_text_changed():
	if action_set:
		action_set.priority = priority_node.text.to_int()

		# just in case we couldn't set this priority, make sure we update our text
		priority_node.text = str(action_set.priority)

func _on_Expanded_pressed():
	is_expanded = !is_expanded
	_update_expanded()
