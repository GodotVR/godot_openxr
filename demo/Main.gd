extends Spatial

func _ready():
	# Start our OpenXR session
	$FPSController.initialise()

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
		$FPSController.rotation.y += delta
	elif (Input.is_key_pressed(KEY_RIGHT)):
		$FPSController.rotation.y -= delta

	if (Input.is_key_pressed(KEY_UP)):
		$FPSController.translation -= $FPSController.transform.basis.z * delta;
	elif (Input.is_key_pressed(KEY_DOWN)):
		$FPSController.translation += $FPSController.transform.basis.z * delta;
