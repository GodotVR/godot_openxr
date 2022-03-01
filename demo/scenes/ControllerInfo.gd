extends Node2D

var controller : ARVRController = null;
var configuration

# Called when the node enters the scene tree for the first time.
func _ready():
	# a little dirty but the parent of the parent of the parent should be the controller.
	controller = get_node("../../../");

	# and just go from there to get our configuration node
	configuration = controller.get_node("../Configuration")

func _process(_delta : float):
	if controller:
		var trigger = controller.get_joystick_axis(JOY_VR_ANALOG_TRIGGER)
		$Container/TriggerInput.value = int(trigger * 100.0)

		var grip = controller.get_joystick_axis(JOY_VR_ANALOG_GRIP)
		$Container/GripInput.value = int(grip * 100.0)

		var joy_x = controller.get_joystick_axis(JOY_AXIS_0)
		var joy_y = controller.get_joystick_axis(JOY_AXIS_1)
		$Container/Joysticks/Primary/Background.color = Color(1.0, 1.0, 1.0, 1.0) if controller.is_button_pressed(JOY_BUTTON_12) else Color(0.7, 0.7, 0.7, 1.0)
		$Container/Joysticks/Primary/Background/Puck.rect_position = Vector2(23 + joy_x * 23, 23 - joy_y * 23)
		$Container/Joysticks/Primary/Background/Puck.color = Color(0.0, 0.0, 1.0, 1.0) if controller.is_button_pressed(JOY_BUTTON_14) else Color(0.0, 0.0, 0.0, 1.0)

		joy_x = controller.get_joystick_axis(JOY_AXIS_6)
		joy_y = controller.get_joystick_axis(JOY_AXIS_7)
		$Container/Joysticks/Secondary/Background.color = Color(1.0, 1.0, 1.0, 1.0) if controller.is_button_pressed(JOY_BUTTON_11) else Color(0.7, 0.7, 0.7, 1.0)
		$Container/Joysticks/Secondary/Background/Puck.rect_position = Vector2(23 + joy_x * 23, 23 - joy_y * 23)
		$Container/Joysticks/Secondary/Background/Puck.color = Color(0.0, 0.0, 1.0, 1.0) if controller.is_button_pressed(JOY_BUTTON_13) else Color(0.0, 0.0, 0.0, 1.0)

		$Container/AXButton/Value.pressed = controller.is_button_pressed(JOY_BUTTON_7)
		$Container/BYButton/Value.pressed = controller.is_button_pressed(JOY_BUTTON_1)
		$Container/MenuButton/Value.pressed = controller.is_button_pressed(JOY_BUTTON_3)
		$Container/SelectButton/Value.pressed = controller.is_button_pressed(JOY_BUTTON_4)
		$Container/TriggerButton/Value.pressed = controller.is_button_pressed(JOY_VR_TRIGGER)
		$Container/SideButton/Value.pressed = controller.is_button_pressed(JOY_VR_GRIP)

		if configuration:
			var confidence = configuration.get_tracking_confidence(controller.controller_id)
			if confidence == 0:
				$Container/Tracking.text = "Not tracking"
			elif confidence == 1:
				$Container/Tracking.text = "Low confidence"
			elif confidence == 2:
				$Container/Tracking.text = "High confidence"
			else:
				$Container/Tracking.text = "Unknown tracking status"
