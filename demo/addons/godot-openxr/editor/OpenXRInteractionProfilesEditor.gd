@tool
extends MarginContainer

var action_sets : OpenXRActionSets = null
var profiles : Array
var interaction_profile_editor : OpenXRInteractionProfileBase = null

# These are the interaction profiles for which we have dedicated interfaces
@onready var interaction_profile_defs : Array = [
	{
		"name": "HTC Vive Controller",
		"path": "/interaction_profiles/htc/vive_controller",
		"editor": preload("res://addons/godot-openxr/editor/interaction_profiles/OpenXRHTCViveEditor.tscn")
	},
	{
		"name": "Oculus Touch Controller",
		"path": "/interaction_profiles/oculus/touch_controller",
		"editor": preload("res://addons/godot-openxr/editor/interaction_profiles/OpenXROculusTouchEditor.tscn")
	}
]

# This is our default generic editor for any profile we don't have an editor for
@onready var generic_editor = preload("res://addons/godot-openxr/editor/interaction_profiles/OpenXRGenericEditor.tscn")

# Handy references
@onready var interaction_profiles_dropdown : OptionButton = $Container/Header/InteractionProfiles

func set_edit_action_sets(p_action_sets : OpenXRActionSets):
	if action_sets == p_action_sets:
		return

	if action_sets:
		# remove current signals
		action_sets.disconnect("interaction_profiles_changed", _update_interaction_profiles)

	action_sets = p_action_sets
	profiles = []

	if action_sets:
		# add new signals
		action_sets.connect("interaction_profiles_changed", _update_interaction_profiles)

	# setup our interface
	_update_interaction_profiles()

func _get_name_for_profile(p_path):
	for ip_def in interaction_profile_defs:
		if ip_def['path'] == p_path:
			return ip_def['name']

	# couldn't find it? just return the path
	return p_path

func _get_editor_for_profile(p_path) -> OpenXRInteractionProfileBase:
	for ip_def in interaction_profile_defs:
		if ip_def['path'] == p_path:
			return ip_def['editor'].instantiate()

	# couldn't find it? just return the generic one
	return generic_editor.instantiate()

func _update_interaction_profiles():
	print("Updating profiles...")
	interaction_profiles_dropdown.clear()

	var current_profile_path = ""
	var current_profile_id = -1
	if interaction_profile_editor:
		current_profile_path = interaction_profile_editor.get_profile_path()

	profiles = action_sets.interaction_profiles
	if profiles.size() == 0:
		_change_profile_editor(null)
		return

	for i in profiles.size():
		var profile = profiles[i]
		if current_profile_path == profile.path:
			current_profile_id = i
		var name = _get_name_for_profile(profile.path)
		print("adding " + name)
		interaction_profiles_dropdown.add_item(name, i)

	if current_profile_id >= 0:
		interaction_profiles_dropdown.selected = current_profile_id
	else:
		interaction_profiles_dropdown.selected = 0

	var profile = profiles[interaction_profiles_dropdown.selected]
	if current_profile_path != profile.path:
		var editor : OpenXRInteractionProfileBase = _get_editor_for_profile(profile.path)
		_change_profile_editor(editor)

	if interaction_profile_editor:
		interaction_profile_editor.set_interaction_profile(profile)

func _change_profile_editor(p_new_editor : OpenXRInteractionProfileBase):
	if interaction_profile_editor:
		# unload the old
		$Container.remove_child(interaction_profile_editor)
		interaction_profile_editor.queue_free()
		interaction_profile_editor = null

	interaction_profile_editor = p_new_editor

	if interaction_profile_editor:
		$Container.add_child(interaction_profile_editor)

func _on_NewInteractionProfileBtn_pressed():
	# need to make this work..
	print("open")
	$NewPopup.popup_centered()

func _on_InteractionProfiles_item_selected(index):
	var current_profile_path : String = ""
	if interaction_profile_editor:
		current_profile_path = interaction_profile_editor.get_profile_path()

	print("Current profile path " + current_profile_path)

	var profile = profiles[index]
	if current_profile_path != profile.path:
		print("Changing to new profile path " + profile.path)
		var editor : OpenXRInteractionProfileBase = _get_editor_for_profile(profile.path)
		_change_profile_editor(editor)

	if interaction_profile_editor:
		interaction_profile_editor.set_interaction_profile(profile)
