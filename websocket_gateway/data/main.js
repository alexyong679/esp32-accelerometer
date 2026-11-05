// Declare Three.js variables
let sceneA, cameraA, rendererA, cubeA;
let sceneB, cameraB, rendererB, cubeB;

// Declare WebSocket variable
let websocket;

// Start WebSocket connection on page load
window.addEventListener('load', () => {
    wsConnect("ws://192.168.4.1:81/");
    init3D_A();
    init3D_B();
    onWindowResize();
    animate();
    window.addEventListener('resize', onWindowResize, false);
});

/** -----------------------
 * WebSocket Handling
 * ------------------------
 */

function wsConnect(url) {
    websocket = new WebSocket(url);

    websocket.onopen = (evt) => {
        console.log("WebSocket connected.");
    };

    websocket.onclose = (evt) => {
        console.log("WebSocket closed.");
        // Attempt reconnect after a delay if desired:
        setTimeout(() => wsConnect(url), 2000);
    };

    websocket.onmessage = (evt) => {
        onMessage(evt);
    };

    websocket.onerror = (evt) => {
        console.error("WebSocket error:", evt);
    };
}

function onMessage(evt) {
    console.log("Received message:", evt.data);
    try {
        const obj = JSON.parse(evt.data);

        if (obj.esp === "A") {
            if (obj.type === "angles") {
                updateAngles("A", obj);
                cubeA.rotation.x = obj.pitch * Math.PI / 180;
                cubeA.rotation.y = obj.yaw * Math.PI / 180;
                cubeA.rotation.z = obj.roll * Math.PI / 180;
            } else if (obj.type === "accel") {
                updateAccel("A", obj);
            }
        } else if (obj.esp === "B") {
            if (obj.type === "angles") {
                updateAngles("B", obj);
                cubeB.rotation.x = obj.pitch * Math.PI / 180;
                cubeB.rotation.y = obj.yaw * Math.PI / 180;
                cubeB.rotation.z = obj.roll * Math.PI / 180;
            } else if (obj.type === "accel") {
                updateAccel("B", obj);
            }
        }
    } catch (error) {
        console.error("JSON parse error:", error);
    }
}

function doSend(message) {
    console.log("Sending: ", message);
    websocket.send(message);
}

/** -----------------------
 * Update UI Functions
 * ------------------------
 */

function updateAngles(esp, obj) {
    document.getElementById(`pitch${esp}`).textContent = obj.pitch.toFixed(3);
    document.getElementById(`roll${esp}`).textContent = obj.roll.toFixed(3);
    document.getElementById(`yaw${esp}`).textContent = obj.yaw.toFixed(3);
}

function updateAccel(esp, obj) {
    document.getElementById(`accX${esp}`).textContent = obj.accX.toFixed(3);
    document.getElementById(`accY${esp}`).textContent = obj.accY.toFixed(3);
    document.getElementById(`accZ${esp}`).textContent = obj.accZ.toFixed(3);
}

/** -----------------------
 * Three.js Cube
 * ------------------------
 */

function init3D_A() {
    const containerA = document.getElementById("cube-container-A");

    sceneA = new THREE.Scene();
    sceneA.background = new THREE.Color(0xffffff);

    cameraA = new THREE.PerspectiveCamera(
        75,
        containerA.clientWidth / containerA.clientHeight,
        0.1,
        1000
    );

    rendererA = new THREE.WebGLRenderer({ antialias: true });
    rendererA.setSize(containerA.clientWidth, containerA.clientHeight);
    containerA.appendChild(rendererA.domElement);

    const geometry = new THREE.BoxGeometry(5, 1, 4);
    const cubeMaterials = [
        new THREE.MeshBasicMaterial({ color: 0x03045e }),
        new THREE.MeshBasicMaterial({ color: 0x023e8a }),
        new THREE.MeshBasicMaterial({ color: 0x0077b6 }),
        new THREE.MeshBasicMaterial({ color: 0x03045e }),
        new THREE.MeshBasicMaterial({ color: 0x023e8a }),
        new THREE.MeshBasicMaterial({ color: 0x0077b6 })
    ];

    // just pass the array directly
    cubeA = new THREE.Mesh(geometry, cubeMaterials);
    sceneA.add(cubeA);

    cameraA.position.z = 10;
    rendererA.render(sceneA, cameraA);
}

function init3D_B() {
    const containerB = document.getElementById("cube-container-B");

    sceneB = new THREE.Scene();
    sceneB.background = new THREE.Color(0xffffff);

    cameraB = new THREE.PerspectiveCamera(
        75,
        containerB.clientWidth / containerB.clientHeight,
        0.1,
        1000
    );

    rendererB = new THREE.WebGLRenderer({ antialias: true });
    rendererB.setSize(containerB.clientWidth, containerB.clientHeight);
    containerB.appendChild(rendererB.domElement);

    const geometry = new THREE.BoxGeometry(5, 1, 4);
    const cubeMaterials = [
        new THREE.MeshBasicMaterial({ color: 0x03045e }),
        new THREE.MeshBasicMaterial({ color: 0x023e8a }),
        new THREE.MeshBasicMaterial({ color: 0x0077b6 }),
        new THREE.MeshBasicMaterial({ color: 0x03045e }),
        new THREE.MeshBasicMaterial({ color: 0x023e8a }),
        new THREE.MeshBasicMaterial({ color: 0x0077b6 })
    ];

    // just pass the array directly
    cubeB = new THREE.Mesh(geometry, cubeMaterials);
    sceneB.add(cubeB);

    cameraB.position.z = 10;
    rendererB.render(sceneB, cameraB);
}



function onWindowResize() {
    const containerA = document.getElementById("cube-container-A");
    cameraA.aspect = containerA.clientWidth / containerA.clientHeight;
    cameraA.updateProjectionMatrix();
    rendererA.setSize(containerA.clientWidth, containerA.clientHeight);

    const containerB = document.getElementById("cube-container-B");
    cameraB.aspect = containerB.clientWidth / containerB.clientHeight;
    cameraB.updateProjectionMatrix();
    rendererB.setSize(containerB.clientWidth, containerB.clientHeight);
}

function animate() {
    requestAnimationFrame(animate);
    rendererA.render(sceneA, cameraA);
    rendererB.render(sceneB, cameraB);
}

/** -----------------------
 * Reset Button
 * ------------------------
 */

function resetPosition(espID) {
    // Send a reset message to your ESP over WebSocket
    const msg = JSON.stringify({
        esp: espID,
        command: "reset"
    });
    doSend(msg);
    console.log(`Reset command sent for ESP_${espID}`);
}
