extends Node2D

var controller : XRController3D = null;

# Called when the node enters the scene tree for the first time.
func _ready():
	# a little dirty but the parent of the parent of the parent should be the controller.
	controller = get_node("../../../");

func _process(_delta : float):
	if controller:
		var trigger = controller.get_value("analog_trigger")
		$Container/TriggerInput.value = int(trigger * 100.0)

		var grip = controller.get_value("analog_grip")
		$Container/GripInput.value = int(grip * 100.0)

		var joy = controller.get_axis("primary")
		$Container/Joysticks/Primary/Background.color = Color(1.0, 1.0, 1.0, 1.0) if controller.is_button_pressed("primary_touch") else Color(0.7, 0.7, 0.7, 1.0)
		$Container/Joysticks/Primary/Background/Puck.rect_position = Vector2(23 + joy.x * 23, 23 - joy.y * 23)
		$Container/Joysticks/Primary/Background/Puck.color = Color(0.0, 0.0, 1.0, 1.0) if controller.is_button_pressed("primary_click") else Color(0.0, 0.0, 0.0, 1.0)

		joy = controller.get_axis("secondary")
		$Container/Joysticks/Secondary/Background.color = Color(1.0, 1.0, 1.0, 1.0) if controller.is_button_pressed("secondary_touch") else Color(0.7, 0.7, 0.7, 1.0)
		$Container/Joysticks/Secondary/Background/Puck.rect_position = Vector2(23 + joy.x * 23, 23 - joy.y * 23)
		$Container/Joysticks/Secondary/Background/Puck.color = Color(0.0, 0.0, 1.0, 1.0) if controller.is_button_pressed("secondary_click") else Color(0.0, 0.0, 0.0, 1.0)

		$Container/AXButton/Value.pressed = controller.is_button_pressed("button_ax")
		$Container/BYButton/Value.pressed = controller.is_button_pressed("button_by")
		$Container/MenuButton/Value.pressed = controller.is_button_pressed("button_menu")
		$Container/SelectButton/Value.pressed = controller.is_button_pressed("button_select")
		$Container/TriggerButton/Value.pressed = controller.is_button_pressed("trigger")
		$Container/SideButton/Value.pressed = controller.is_button_pressed("grip")
