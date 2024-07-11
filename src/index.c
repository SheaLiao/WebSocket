/* This file is convert from HTML by tools/convert.sh */
const char *g_index_html=
"<!DOCTYPE html>\r\n"
"<html lang=\"en\">\r\n"
"<head>\r\n"
"    <meta charset=\"UTF-8\">\r\n"
"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
"    <title>Temperature Chart and Light Control</title>\r\n"
"    <link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.1/dist/css/bootstrap.min.css\" rel=\"stylesheet\" integrity=\"sha384-iYQeCzEYFbKjA/T2uDLTpkwGzCiq6soy8tYaI1GyVh/UjpbCx/TYkiZhlZB6+fzT\" crossorigin=\"anonymous\">\r\n"
"    <script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.2.1/dist/js/bootstrap.bundle.min.js\" integrity=\"sha384-u1OknCvxWvY5kfmNBILK2hRnQC3Pr17a+RTT6rIHI7NnikvbZlHgTPOOmMi466C8\" crossorigin=\"anonymous\"></script>\r\n"
"    <script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\r\n"
"    <script src=\"https://cdn.jsdelivr.net/npm/moment@2.29.1/min/moment.min.js\"></script>\r\n"
"    <script src=\"https://cdn.jsdelivr.net/npm/chartjs-adapter-moment@^1.0.0\"></script>\r\n"
"    <style>\r\n"
"        body {\r\n"
"            font-family: Arial, sans-serif;\r\n"
"            display: flex;\r\n"
"            flex-direction: column;\r\n"
"            align-items: center;\r\n"
"            margin: 0;\r\n"
"            padding: 20px;\r\n"
"            background-color: #f0f0f0;\r\n"
"        }\r\n"
"        h1, h2 {\r\n"
"            margin-bottom: 20px;\r\n"
"        }\r\n"
"        canvas {\r\n"
"            max-width: 35%;\r\n"
"            max-height: 55%;\r\n"
"            margin-bottom: 10px;\r\n"
"            border: 2px solid #000;\r\n"
"            padding: 10px;\r\n"
"        }\r\n"
"        #ledContainer {\r\n"
"            display: flex;\r\n"
"            justify-content: center;\r\n"
"            gap: 20px;\r\n"
"        }\r\n"
"        .ledButton {\r\n"
"            width: 60px;\r\n"
"            height: 60px;\r\n"
"            border-radius: 50%;\r\n"
"            border: none;\r\n"
"            font-size: 16px;\r\n"
"            color: white;\r\n"
"            cursor: pointer;\r\n"
"            transition: background-color 0.3s;\r\n"
"        }\r\n"
"        .ledButton.off {\r\n"
"            background-color: gray;\r\n"
"        }\r\n"
"        .ledButton.on-red {\r\n"
"            background-color: red;\r\n"
"        }\r\n"
"        .ledButton.on-green {\r\n"
"            background-color: green;\r\n"
"        }\r\n"
"        .ledButton.on-blue {\r\n"
"            background-color: blue;\r\n"
"        }\r\n"
"        .message {\r\n"
"            display: none;\r\n"
"            margin-top: 20px;\r\n"
"            padding: 10px;\r\n"
"            background-color: #333;\r\n"
"            color: #fff;\r\n"
"            border-radius: 5px;\r\n"
"            font-size: 18px;\r\n"
"            position: fixed;\r\n"
"            top: 20px;\r\n"
"            left: 50%;\r\n"
"            transform: translateX(-50%);\r\n"
"            z-index: 1000;\r\n"
"        }\r\n"
"    </style>\r\n"
"</head>\r\n"
"<body>\r\n"
"    <h1>Temperature</h1>\r\n"
"    <canvas id=\"temperatureChart\"></canvas>\r\n"
"    <h2>LED Control</h2>\r\n"
"    <div id=\"ledContainer\">\r\n"
"        <button id=\"LED_R\" class=\"ledButton off\" onclick=\"toggleLight('LED_R')\">OFF</button>\r\n"
"        <button id=\"LED_G\" class=\"ledButton off\" onclick=\"toggleLight('LED_G')\">OFF</button>\r\n"
"        <button id=\"LED_B\" class=\"ledButton off\" onclick=\"toggleLight('LED_B')\">OFF</button>\r\n"
"    </div>\r\n"
"    <div id=\"message\" class=\"message\">提示信息</div>\r\n"
"    <script type=\"text/javascript\">\r\n"
"        document.addEventListener(\"DOMContentLoaded\", function() {\r\n"
"            const ctx = document.getElementById('temperatureChart').getContext('2d');\r\n"
"            const temperatureData = {\r\n"
"                labels: [],\r\n"
"                datasets: [{\r\n"
"                    label: 'Temperature',\r\n"
"                    data: [],\r\n"
"                    borderColor: 'rgba(75, 192, 192, 1)',\r\n"
"                    borderWidth: 1,\r\n"
"                    fill: false\r\n"
"                }]\r\n"
"            };\r\n"
"            const config = {\r\n"
"                type: 'line',\r\n"
"                data: temperatureData,\r\n"
"                options: {\r\n"
"                    scales: {\r\n"
"                        x: {\r\n"
"                            type: 'time',\r\n"
"                            time: {\r\n"
"                                unit: 'minute'\r\n"
"                            }\r\n"
"                        },\r\n"
"                        y: {\r\n"
"                            beginAtZero: true\r\n"
"                        }\r\n"
"                    }\r\n"
"                }\r\n"
"            };\r\n"
"            const temperatureChart = new Chart(ctx, config);\r\n"
"            const ledColors = {\r\n"
"                'LED_R': 'red',\r\n"
"                'LED_G': 'green',\r\n"
"                'LED_B': 'blue'\r\n"
"            };\r\n"
"            const socket = new WebSocket('ws://studio.weike-iot.com:3333/');\r\n"
"            socket.onopen = function() {\r\n"
"                console.log('WebSocket connection established');\r\n"
"            };\r\n"
"            socket.onerror = function(error) {\r\n"
"                console.error('WebSocket connection error:', error);\r\n"
"            };\r\n"
"            socket.onmessage = function(event) {\r\n"
"                const message = JSON.parse(event.data);\r\n"
"                if (message.type === 'temperature') {\r\n"
"                    const time = new Date(message.timestamp);\r\n"
"                    temperatureData.labels.push(time);\r\n"
"                    temperatureData.datasets[0].data.push(message.value);\r\n"
"                    if (temperatureData.labels.length > 50) {\r\n"
"                        temperatureData.labels.shift();\r\n"
"                        temperatureData.datasets[0].data.shift();\r\n"
"                    }\r\n"
"                    temperatureChart.update();\r\n"
"                } else if (message.type === 'ledStatus') {\r\n"
"                    const led = document.getElementById(`LED_${message.ledId}`);\r\n"
"                    const color = ledColors[`LED_${message.ledId}`];\r\n"
"                    if (message.status === 'on') {\r\n"
"                        led.classList.remove('off');\r\n"
"                        led.classList.add(`on-${color}`);\r\n"
"                        led.textContent = 'ON';\r\n"
"                    } else {\r\n"
"                        led.classList.remove(`on-${color}`);\r\n"
"                        led.classList.add('off');\r\n"
"                        led.textContent = 'OFF';\r\n"
"                    }\r\n"
"                }\r\n"
"            };\r\n"
"            window.toggleLight = function(ledId) {\r\n"
"                const led = document.getElementById(ledId);\r\n"
"                const color = ledColors[ledId];\r\n"
"                const messageElement = document.getElementById('message');\r\n"
"                let action;\r\n"
"                let status;\r\n"
"                if (led.classList.contains('off')) {\r\n"
"                    led.classList.remove('off');\r\n"
"                    led.classList.add(`on-${color}`);\r\n"
"                    led.textContent = 'ON';\r\n"
"                    action = '打开';\r\n"
"                    status = 'on';\r\n"
"                } else {\r\n"
"                    led.classList.remove(`on-${color}`);\r\n"
"                    led.classList.add('off');\r\n"
"                    led.textContent = 'OFF';\r\n"
"                    action = '关闭';\r\n"
"                    status = 'off';\r\n"
"                }\r\n"
"                // 显示提示信息\r\n"
"                messageElement.textContent = `${action} LED${ledId.charAt(ledId.length - 1)}`;\r\n"
"                messageElement.style.display = 'block';\r\n"
"                // 3秒后隐藏提示信息\r\n"
"                setTimeout(function() {\r\n"
"                    messageElement.style.display = 'none';\r\n"
"                }, 3000);\r\n"
"                // 发送LED状态到服务器\r\n"
"                socket.send(JSON.stringify({\r\n"
"                    type: 'toggleLED',\r\n"
"                    ledId: ledId.charAt(ledId.length - 1),\r\n"
"                    status: status\r\n"
"                }));\r\n"
"            };\r\n"
"        });\r\n"
"    </script>\r\n"
"</body>\r\n"
"</html>\r\n"
;
