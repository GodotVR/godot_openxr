extends Node2D

var controller : ARVRController = null;

# Called when the node enters the scene tree for the first time.
func _ready():
	# a little dirty but the parent of the parent of the parent should be the controller.
	controller = get_node("../../../");

func _process(_delta : float):
	if controller:
		var trigger = controller.get_joystick_axis(JOY_VR_ANALOG_TRIGGER)
		$Container/TriggerInput.value = int(trigger * 100.0)

		var grip = controller.get_joystick_axis(JOY_VR_ANALOG_GRIP)
		$Container/GripInput.value = int(grip * 100.0)

		var joy_x = controller.get_joystick_axis(JOY_AXIS_0)
		var joy_y = controller.get_joystick_axis(JOY_AXIS_1)
		$Container/Control/Joystick/Puck.rect_position = Vector2(23 + joy_x * 23, 23 - joy_y * 23)

		$Container/AXButton/Value.pressed = controller.is_button_pressed(JOY_BUTTON_7)
		$Container/BYMButton/Value.pressed = controller.is_button_pressed(JOY_BUTTON_1)
		$Container/TriggerButton/Value.pressed = controller.is_button_pressed(JOY_VR_TRIGGER)
		$Container/SideButton/Value.pressed = controller.is_button_pressed(JOY_VR_GRIP)
