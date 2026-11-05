let scene, camera, renderer, cube;

// Helper functions to get parent dimensions for Three.js canvas resizing
function parentWidth(elem) {
  return elem.parentElement.clientWidth;
}

function parentHeight(elem) {
  return elem.parentElement.clientHeight;
}

/**
this function sets up a 3D cube
scene = holds all the 3d obejcts
camera = ur eyes in the scene
renderer = draws the 3d scene to the webpage
cube = the obj that will rotate when u turn on the sensor
 */
function init3D(){
  // Create a new Three.js scene
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0xffffff); // White background

  // Create a perspective camera
  // Arguments: FOV, Aspect Ratio, Near, Far clipping plane
  camera = new THREE.PerspectiveCamera(
    75,
    parentWidth(document.getElementById("3Dcube")) / parentHeight(document.getElementById("3Dcube")),
    0.1,
    1000
  );

  // Create a WebGL renderer with anti-aliasing for smoother edges
  renderer = new THREE.WebGLRenderer({ antialias: true });
  // Set renderer size to match the parent container
  renderer.setSize(parentWidth(document.getElementById("3Dcube")), parentHeight(document.getElementById("3Dcube")));

  // Append the renderer's DOM element (canvas) to the #3Dcube div
  document.getElementById('3Dcube').appendChild(renderer.domElement);

  // Create a cube geometry (width, height, depth)
  const geometry = new THREE.BoxGeometry(5, 1, 4);

  // Define materials for each face of the cube
  // Using different shades of blue for visual distinction
  var cubeMaterials = [
    new THREE.MeshBasicMaterial({color:0x03045e}), // Right face
    new THREE.MeshBasicMaterial({color:0x023e8a}), // Left face
    new THREE.MeshBasicMaterial({color:0x0077b6}), // Top face
    new THREE.MeshBasicMaterial({color:0x03045e}), // Bottom face
    new THREE.MeshBasicMaterial({color:0x023e8a}), // Front face
    new THREE.MeshBasicMaterial({color:0x0077b6}), // Back face
  ];

  // Apply materials to the cube
  const material = new THREE.MeshFaceMaterial(cubeMaterials);

  // Create the cube mesh
  cube = new THREE.Mesh(geometry, material);
  scene.add(cube); // Add the cube to the scene

  camera.position.z = 5; // Position the camera to see the cube
  renderer.render(scene, camera); // Render the initial scene
}

/**
 * @brief Resizes the Three.js canvas and updates camera aspect ratio when the window is resized.
 * This ensures the 3D cube scales correctly with the browser window.
 */
function onWindowResize(){
  // Update camera aspect ratio based on the parent container's new dimensions
  camera.aspect = parentWidth(document.getElementById("3Dcube")) / parentHeight(document.getElementById("3Dcube"));
  camera.updateProjectionMatrix(); // Recalculate projection matrix for new aspect ratio

  // Update renderer size
  renderer.setSize(parentWidth(document.getElementById("3Dcube")), parentHeight(document.getElementById("3Dcube")));
  renderer.render(scene, camera); // Re-render the scene with new dimensions
}

// Add event listener for window resize events
window.addEventListener('resize', onWindowResize, false);

// Initialize the 3D cube when the script loads
init3D();

// --- Server-Sent Events (SSE) Handling ---
// this connects the js to the esp32
if (!!window.EventSource) {
  var source = new EventSource('/events');
  const statusElement = document.getElementById('status');

  // Event listener for when the SSE connection is opened
  source.addEventListener('open', function(e) {
    console.log("Events Connected");
    statusElement.textContent = "Connected";
    statusElement.classList.remove('text-red-500', 'text-gray-500');
    statusElement.classList.add('text-green-500');
  }, false);

  // Event listener for SSE errors or disconnection
  source.addEventListener('error', function(e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
      statusElement.textContent = "Disconnected";
      statusElement.classList.remove('text-green-500', 'text-gray-500');
      statusElement.classList.add('text-red-500');
    } else {
      console.error("SSE Error:", e);
      statusElement.textContent = "Error";
      statusElement.classList.remove('text-green-500', 'text-gray-500');
      statusElement.classList.add('text-red-500');
    }
  }, false);

  /**
   * @brief Event listener for 'angles' data from the ESP32.
   * Updates the displayed Pitch, Roll, Yaw angles and rotates the 3D cube.
   * The ESP32 sends angles in degrees, Three.js expects radians.
   */
  source.addEventListener('angles', function(e) {
    var obj = JSON.parse(e.data);
    // Update angle display elements
    document.getElementById("pitch").innerHTML = parseFloat(obj.pitch).toFixed(3);
    document.getElementById("roll").innerHTML = parseFloat(obj.roll).toFixed(3);
    document.getElementById("yaw").innerHTML = parseFloat(obj.yaw).toFixed(3);

    // Update cube rotation (convert degrees to radians: angle * Math.PI / 180)
    // Common mapping: pitch to X-axis, roll to Z-axis, yaw to Y-axis
    cube.rotation.x = obj.pitch * Math.PI / 180;
    cube.rotation.z = obj.roll * Math.PI / 180;
    cube.rotation.y = obj.yaw * Math.PI / 180; // Yaw will drift without magnetometer

    renderer.render(scene, camera); // Re-render the scene to show new rotation
  }, false);

  /**
   * @brief Event listener for 'gyro_readings' data from the ESP32.
   * Updates the displayed raw gyroscope angular velocities.
   
  source.addEventListener('gyro_readings', function(e) {
    var obj = JSON.parse(e.data);
    // Update gyroscope display elements (in deg/s)
    document.getElementById("gyroX").innerHTML = parseFloat(obj.gyroX).toFixed(3);
    document.getElementById("gyroY").innerHTML = parseFloat(obj.gyroY).toFixed(3);
    document.getElementById("gyroZ").innerHTML = parseFloat(obj.gyroZ).toFixed(3);
  }, false);

  im not sending gyro data anymore tho i might change that

  /*

  /**
   * @brief Event listener for 'accelerometer_readings' data from the ESP32.
   * Updates the displayed accelerometer readings.
   */
  source.addEventListener('accel', function(e) {
    var obj = JSON.parse(e.data);
    // Update accelerometer display elements (in g)
    document.getElementById("accX").innerHTML = parseFloat(obj.accX).toFixed(3);
    document.getElementById("accY").innerHTML = parseFloat(obj.accY).toFixed(3);
    document.getElementById("accZ").innerHTML = parseFloat(obj.accZ).toFixed(3);
  }, false);

}

/**
 * @brief Sends a request to the ESP32 to reset the calculated angles.
 * This function is called when the "RESET ANGLES" button is clicked.
 */
function resetPosition(){ // Renamed function for clarity
  var xhr = new XMLHttpRequest();
  xhr.open("GET", "/reset", true); // Send GET request to /reset endpoint
  xhr.send();
  console.log("Resetting angles on ESP32...");
}

// Attach the resetPosition function to the 'click' event of the reset button
document.getElementById('resetButton').addEventListener('click', resetPosition);
