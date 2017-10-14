extends Spatial

var arvr_interface = null

func _ready():
	# we should move this code into a support file of sorts so it can be loaded in autoload
	arvr_interface = ARVRInterfaceGDNative.new()
	arvr_interface.set_gdnative_library(preload("res://godot_openhmd.tres"))

	# and then just find the interface
	#	arvr_interface = ARVRServer.find_interface("godot_openhmd")
	if arvr_interface and arvr_interface.initialize():
		get_viewport().arvr = true

func _process(delta):
	# Test for escape to close application, space to reset our reference frame
	if (Input.is_key_pressed(KEY_ESCAPE)):
		get_tree().quit()
	elif (Input.is_key_pressed(KEY_SPACE)):
		# Calling center_on_hmd will cause the ARVRServer to adjust all tracking data so the player is centered on the origin point looking forward
		ARVRServer.center_on_hmd(true, true)

	# We minipulate our origin point to move around. Note that with roomscale tracking a little more then this is needed
	# because we'll rotate around our origin point, not around our player. But that is a subject for another day.
	if (Input.is_key_pressed(KEY_LEFT)):
		$ARVROrigin.rotation.y += delta
	elif (Input.is_key_pressed(KEY_RIGHT)):
		$ARVROrigin.rotation.y -= delta

	if (Input.is_key_pressed(KEY_UP)):
		$ARVROrigin.translation -= $ARVROrigin.transform.basis.z * delta;
	elif (Input.is_key_pressed(KEY_DOWN)):
		$ARVROrigin.translation += $ARVROrigin.transform.basis.z * delta;