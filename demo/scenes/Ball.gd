extends XRToolsPickable

class_name BasketBall

var reset_transform : Transform
var resetting = false
var can_bounce = true

# Called when the node enters the scene tree for the first time.
func _ready():
	reset_transform = global_transform

func pick_up(by, with_controller):
	$AnimationPlayer.stop()
	resetting = false

	.pick_up(by, with_controller)

func let_go(p_linear_velocity = Vector3(), p_angular_velocity = Vector3()):
	.let_go(p_linear_velocity, p_angular_velocity)

	$AnimationPlayer.play("ResetAnim")
	resetting = true;

func _reset():
	$AnimationPlayer.stop()
	resetting = false

	angular_velocity = Vector3()
	linear_velocity = Vector3()
	global_transform = reset_transform
	visible = true

func _process(delta):
	if resetting:
		return

	var distance = (global_transform.origin - reset_transform.origin).length()
	if distance > 5.0:
		# rolled away too far? reset it!
		$AnimationPlayer.play("ResetAnim")
		resetting = true

func get_full_name(node):
	var full_name = node.get_name()
	var parent = node.get_parent()
	while parent:
		full_name = parent.get_name() + "/" + full_name
		parent = parent.get_parent()

	return full_name

func _on_Ball_body_shape_entered(body_rid, body, body_shape_index, local_shape_index):
	var hit_name = get_full_name(body)
	var velocity = linear_velocity.length()

	# Shame we're not getting info about our collision,
	# we need the normal at our collision to see how hard we collided.
	# Now we can get bounce sounds simply for rolling on the floor.

	# For this we'll make some special checks just for our court and ground
	if hit_name == "root/Main/BallCourt" or hit_name == "root/Main/Ground":
		velocity = abs(linear_velocity.dot(Vector3.UP))

	print("hit %s at velocity %.2f" % [ hit_name, velocity ])

	if velocity > 0.5 and can_bounce:
		$AudioStreamPlayer3D.play()
		can_bounce = false
		$BounceTimer.start()

func _on_BounceTimer_timeout():
	can_bounce = true
