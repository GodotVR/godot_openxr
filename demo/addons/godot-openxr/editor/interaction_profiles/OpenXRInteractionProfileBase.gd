@tool
extends MarginContainer

class_name OpenXRInteractionProfileBase

var interaction_profile : OpenXRInteractionProfile

func get_profile_path() -> String:
	if interaction_profile:
		return interaction_profile.path
	
	return ""

func set_interaction_profile(p_interaction_profile : OpenXRInteractionProfile):
	if interaction_profile == p_interaction_profile:
		return

	if interaction_profile:
		# disconnect
		pass

	interaction_profile = p_interaction_profile

	if interaction_profile:
		# connect
		pass
	
	_update_interaction_profile()

func _update_interaction_profile():
	pass

# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.
