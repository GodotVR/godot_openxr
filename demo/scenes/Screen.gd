extends Node2D

signal toggle_bounds

var count = 0
var fp_controller : ARVROrigin

func set_fp_controller(p_node : ARVROrigin):
	fp_controller = p_node
	if fp_controller and fp_controller.is_passthrough_supported():
		$TogglePassthrough.disabled = false
	else:
		$TogglePassthrough.disabled = true

# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.

func _on_Button_pressed():
	count = count + 1
	$Count.text = "Pressed %d times" % [ count ]

func _on_ToggleBounds_pressed():
	emit_signal("toggle_bounds")

func _on_TogglePassthrough_pressed():
	if fp_controller:
		fp_controller.enable_passthrough = !fp_controller.enable_passthrough
