extends StaticBody

var can_score = true

func _on_BottomDetector_body_entered(body):
	# If we can score, the body is our basketball, and its still touching our top detector, we score
	if can_score and body is BasketBall and $RingMesh/TopDetector.overlaps_body(body):
		var score_scene = $Board.get_scene_instance()
		if score_scene:
			score_scene.inc_score()

		# prevent the same goal to score multiple times
		can_score = false
		$ScoreCooldown.start()

func _on_ScoreCooldown_timeout():
	can_score = true
