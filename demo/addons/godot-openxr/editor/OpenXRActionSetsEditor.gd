@tool
extends ScrollContainer

class_name OpenXRActionSetsEditor

@onready var action_sets_node = $Container/ActionSets/Container
@onready var action_set_scene = preload("res://addons/godot-openxr/editor/OpenXRActionSetEditor.tscn")
var action_sets : OpenXRActionSets = null

func _ready():
	var color = ProjectSettings
	pass

func set_edit_action_sets(p_action_sets : OpenXRActionSets):
	if action_sets == p_action_sets:
		return

	if action_sets:
		# remove current signals
		action_sets.disconnect("action_sets_changed", _update_action_sets)

	action_sets = p_action_sets

	if action_sets:
		# check for default entries
		_check_default_setup()

		# add new signals
		action_sets.connect("action_sets_changed", _update_action_sets)

	# setup our interface
	_update_action_sets()
	$Container/InteractionProfiles.set_edit_action_sets(action_sets)

func _check_default_setup():
	var sets = action_sets.action_sets
	if sets.size() == 0:
		# setup a basic action set with one action
		var new_set = OpenXRActionSet.new()
		new_set.name = "Your action set"
		var new_action = OpenXRAction.new()
		new_action.name = "action"
		new_set.add_action(new_action)

		action_sets.add_action_set(new_set)

	var profiles = action_sets.interaction_profiles
	if profiles.size() == 0:
		# for a new resource we're creating our basic profiles by default
		var new_ip = OpenXRInteractionProfile.new()
		new_ip.path = "/interaction_profiles/khr/simple_controller"
		action_sets.add_interaction_profile(new_ip)

		new_ip = OpenXRInteractionProfile.new()
		new_ip.path = "/interaction_profiles/htc/vive_controller"
		action_sets.add_interaction_profile(new_ip)

		new_ip = OpenXRInteractionProfile.new()
		new_ip.path = "/interaction_profiles/microsoft/motion_controller"
		action_sets.add_interaction_profile(new_ip)

		new_ip = OpenXRInteractionProfile.new()
		new_ip.path = "/interaction_profiles/oculus/touch_controller"
		action_sets.add_interaction_profile(new_ip)

		new_ip = OpenXRInteractionProfile.new()
		new_ip.path = "/interaction_profiles/valve/index_controller"
		action_sets.add_interaction_profile(new_ip)

func _update_action_sets():
	var current_action_sets = action_sets_node.get_children()

	if action_sets:
		var i = 1
		for action_set in action_sets.get_action_sets():
			var node_name = "action_set_" + str(i)
			var node = action_sets_node.get_node_or_null(node_name)
			if node:
				# remove it from our current_action_sets, we are reusing this
				current_action_sets.erase(node)
			else:
				# create a new node
				node = action_set_scene.instantiate()
				node.name = node_name
				action_sets_node.add_child(node)
				node.connect("remove_action_set", _on_RemoveActionSet)

			node.set_action_set(action_set)

			i = i + 1

	# remove the remainders, we don't need them anymore...
	for node in current_action_sets:
		if str(node.name).begins_with("action_set_"):
			action_sets_node.remove_child(node)
			node.queue_free()

func _on_NewActionSetBtn_pressed():
	if action_sets:
		var new_action_set = OpenXRActionSet.new()
		new_action_set.name = "new"
		action_sets.add_action_set(new_action_set)

func _on_RemoveActionSet(p_action_set : OpenXRActionSet):
	if action_sets:
		action_sets.remove_action_set(p_action_set)

func _on_SaveBtn_pressed():
	if action_sets:
		ResourceSaver.save(action_sets.resource_path, action_sets)
