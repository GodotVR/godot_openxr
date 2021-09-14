extends XRController3D

signal activated
signal deactivated

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	if get_is_active():
		if !visible:
			visible = true
			print("Activated " + str(name))
			emit_signal("activated")
	elif visible:
		visible = false
		print("Deactivated " + str(name))
		emit_signal("deactivated")
