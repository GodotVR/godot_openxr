tool
class_name Function_WindMovement
extends MovementProvider

## Signal invoked when changing active wind areas
signal wind_area_changed(active_wind_area)

## Movement provider order
export var order := 25

## Drag multiplier for the player
export var drag_multiplier := 1.0

# Wind area
onready var _sense_area: Area = $Area

# Array of wind areas the player is in
var _in_wind_areas := Array()

# Currently active wind area
var _active_wind_area: WindArea = null

# Called when the node enters the scene tree for the first time.
func _ready():
	# Skip if running in the editor
	if Engine.editor_hint:
		return

	# Reparent the sense area to the camera
	var camera = get_arvr_camera()
	if camera:
		self.remove_child(_sense_area)
		camera.add_child(_sense_area)

	# Subscribe to area notifications
	_sense_area.connect("area_entered", self, "_on_area_entered")
	_sense_area.connect("area_exited", self, "_on_area_exited")

func _on_area_entered(area: Area):
	# Skip if not wind area
	var wind_area = area as WindArea
	if !wind_area:
		return

	# Save area and set active
	_in_wind_areas.push_front(wind_area)
	_active_wind_area = wind_area

	# Report the wind area change
	emit_signal("wind_area_changed", _active_wind_area)

func _on_area_exited(area: Area):
	# Erase from the wind area
	_in_wind_areas.erase(area)
	
	# If we didn't leave the active wind area then we're done
	if area != _active_wind_area:
		return

	# Select a new active wind area
	if _in_wind_areas.empty():
		_active_wind_area = null
	else:
		_active_wind_area = _in_wind_areas.front()

	# Report the wind area change
	emit_signal("wind_area_changed", _active_wind_area)

# Perform jump movement
func physics_movement(delta: float, player_body: PlayerBody):
	# Skip if no active wind area
	if !_active_wind_area:
		return

	# Calculate the global wind velocity of the wind area
	var wind_velocity := _active_wind_area.global_transform.basis.xform(_active_wind_area.wind_vector)

	# Drag the player into the wind
	var drag_factor := _active_wind_area.drag * drag_multiplier * delta
	drag_factor = clamp(drag_factor, 0.0, 1.0)
	player_body.velocity = lerp(player_body.velocity, wind_velocity, drag_factor)

# Get our camera node
func get_arvr_camera() -> ARVRCamera:
	# Get the ARVROrigin node
	var origin := get_arvr_origin()
	if !origin:
		return null

	# Attempt to get using the default name
	var camera := origin.get_node_or_null("ARVRCamera") as ARVRCamera
	if camera:
		return camera

	# Find the first ARVRCamera child
	for child in origin.get_children():
		camera = child as ARVRCamera
		if camera:
			return camera

	# Unable to find ARVRCamera
	return null

# This method verifies the MovementProvider has a valid configuration.
func _get_configuration_warning():
	# Call base class
	return ._get_configuration_warning()
