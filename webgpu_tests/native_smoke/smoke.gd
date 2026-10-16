extends Node3D

var frames := 0


func _ready() -> void:
	if RenderingServer.get_current_rendering_driver_name() != "webgpu":
		push_error("Native WebGPU fell back to another renderer")
		get_tree().quit(1)
		return
	var box := MeshInstance3D.new()
	box.mesh = BoxMesh.new()
	add_child(box)
	var camera := Camera3D.new()
	camera.position = Vector3(0, 0, 3)
	camera.current = true
	add_child(camera)
	var light := DirectionalLight3D.new()
	light.rotation_degrees = Vector3(-30, -30, 0)
	add_child(light)


func _process(_delta: float) -> void:
	frames += 1
	if frames == 8:
		_check_pixels()


func _check_pixels() -> void:
	await RenderingServer.frame_post_draw
	var image := get_viewport().get_texture().get_image()
	if image == null or image.is_empty():
		push_error("Native WebGPU returned no viewport image")
		get_tree().quit(1)
		return
	var center := image.get_pixel(image.get_width() / 2, image.get_height() / 2)
	var background := image.get_pixel(0, 0)
	if center.is_equal_approx(background):
		push_error("Native WebGPU did not render the cube")
		get_tree().quit(1)
		return
	print("NATIVE_WEBGPU_SMOKE_PASSED: center=", center, " background=", background)
	get_tree().quit()
