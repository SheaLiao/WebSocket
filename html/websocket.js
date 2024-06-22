document.addEventListener("DOMContentLoaded", function() {
    const ctx = document.getElementById('temperatureChart').getContext('2d');
    const temperatureData = {
        labels: [],
        datasets: [{
            label: 'Temperature',
            data: [],
            borderColor: 'rgba(75, 192, 192, 1)',
            borderWidth: 1,
            fill: false
        }]
    };
    const config = {
        type: 'line',
        data: temperatureData,
        options: {
            scales: {
                x: {
                    type: 'time',
                    time: {
                        unit: 'minute'
                    }
                },
                y: {
                    beginAtZero: true
                }
            }
        }
    };
    const temperatureChart = new Chart(ctx, config);

    const ledColors = {
        'led1': 'red',
        'led2': 'green',
        'led3': 'blue',
        'led4': 'purple'
    };

    const socket = new WebSocket('ws://192.168.10.167:8888/');
    socket.onopen = function() {
        console.log('WebSocket connection established');
    };
    socket.onerror = function(error) {
        console.error('WebSocket connection error:', error);
    };
    

    socket.onmessage = function(event) {
        const message = JSON.parse(event.data);

        if (message.type === 'temperature') {
            const time = new Date(message.timestamp);
            temperatureData.labels.push(time);
            temperatureData.datasets[0].data.push(message.value);

            if (temperatureData.labels.length > 50) {
                temperatureData.labels.shift();
                temperatureData.datasets[0].data.shift();
            }

            temperatureChart.update();
        } else if (message.type === 'ledStatus') {
            const led = document.getElementById(`led${message.ledId}`);
            const color = ledColors[`led${message.ledId}`];
            if (message.status === 'on') {
                led.classList.remove('off');
                led.classList.add(`on-${color}`);
                led.textContent = 'ON';
            } else {
                led.classList.remove(`on-${color}`);
                led.classList.add('off');
                led.textContent = 'OFF';
            }
        }
    };

    window.toggleLight = function(ledId) {
        const led = document.getElementById(ledId);
        const color = ledColors[ledId];
        const messageElement = document.getElementById('message');
        let action;
        let status;

        if (led.classList.contains('off')) {
            led.classList.remove('off');
            led.classList.add(`on-${color}`);
            led.textContent = 'ON';
            action = '打开';
            status = 'on';
        } else {
            led.classList.remove(`on-${color}`);
            led.classList.add('off');
            led.textContent = 'OFF';
            action = '关闭';
            status = 'off';
        }

        // 显示提示信息
        messageElement.textContent = `${action} LED${ledId.charAt(ledId.length - 1)}`;
        messageElement.style.display = 'block';

        // 3秒后隐藏提示信息
        setTimeout(function() {
            messageElement.style.display = 'none';
        }, 3000);

        // 发送LED状态到服务器
        socket.send(JSON.stringify({
            type: 'toggleLED',
            ledId: ledId.charAt(ledId.length - 1),
            status: status
        }));
    };
});
