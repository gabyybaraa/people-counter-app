import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:web_socket_channel/web_socket_channel.dart';
import 'package:web_socket_channel/status.dart' as status;
import 'dart:convert';
import 'dart:async';
import 'dart:io';

// HTTP Override to allow local network connections
class MyHttpOverrides extends HttpOverrides {
  @override
  HttpClient createHttpClient(SecurityContext? context) {
    return super.createHttpClient(context)
      ..badCertificateCallback = (X509Certificate cert, String host, int port) => true;
  }
}

void main() {
  // Allow HTTP requests to local networks
  HttpOverrides.global = MyHttpOverrides();
  runApp(PeopleCounterApp()); // Disables SSL certificate checks
}

class PeopleCounterApp extends StatelessWidget {
  const PeopleCounterApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'People Counter',
      theme: ThemeData(
        primarySwatch: Colors.blue,
        visualDensity: VisualDensity.adaptivePlatformDensity,
      ),
      home: PeopleCounterHomePage(),
      debugShowCheckedModeBanner: false,
    );
  }
}

class PeopleCounterHomePage extends StatefulWidget {
  const PeopleCounterHomePage({super.key});

  @override
  _PeopleCounterHomePageState createState() => _PeopleCounterHomePageState();
}

class _PeopleCounterHomePageState extends State<PeopleCounterHomePage> {
  // Device connection
  String deviceIP = '192.168.4.1';
  bool isConnected = false;
  bool isConnecting = false;
  
  // Counter data
  int currentCount = 0;
  String deviceStatus = 'disconnected';
  String lastUpdateTime = '';
  
  // WebSocket
  WebSocketChannel? webSocketChannel;
  StreamSubscription? webSocketSubscription;
  
  // Statistics
  int totalEntered = 0;
  int totalExited = 0;
  List<CountEvent> countHistory = [];
  
  // Controllers
  final TextEditingController ipController = TextEditingController();
  Timer? reconnectTimer;
  Timer? statusUpdateTimer;

  @override
  void initState() {
    super.initState();
    ipController.text = deviceIP;
    _connectToDevice();
    
    statusUpdateTimer = Timer.periodic(const Duration(seconds: 30), (timer) {
      if (isConnected) {
        _fetchDeviceStatus();
      }
    });
  }

  @override
  void dispose() {
    _disconnectWebSocket();
    reconnectTimer?.cancel();
    statusUpdateTimer?.cancel();
    ipController.dispose();
    super.dispose();
  }

  // Main connection method
  Future<void> _connectToDevice() async {
    setState(() {
      isConnecting = true;
    });

    print('\n=== Starting Connection Process ===');
    print('Target IP: $deviceIP');

    try {
      print('Testing API endpoint...');
      
      final response = await http.get(
        Uri.parse('http://$deviceIP/api/test'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 10));

      print('API Response Status: ${response.statusCode}');
      print('API Response Body: ${response.body}');

      if (response.statusCode == 200) {
        final Map<String, dynamic> data = json.decode(response.body);
        print('Parsed API data: $data');
        
        if (data.containsKey('count') || data.containsKey('status')) {
          await _fetchCurrentCount();
          _connectWebSocket();
          
          setState(() {
            isConnected = true;
            isConnecting = false;
          });
          
          _showSnackBar('Connected successfully! IP: $deviceIP', Colors.green);
          print('=== Connection Successful ===\n');
        } else {
          throw Exception('Device response format unexpected');
        }
      } else {
        throw Exception('Device responded with status: ${response.statusCode}');
      }
    } catch (e) {
      setState(() {
        isConnected = false;
        isConnecting = false;
      });
      
      print('Connection failed: $e');
      
      if (e is SocketException) {
        _showSnackBar('Network error: Check WiFi connection to ESP8266-PeopleCounter', Colors.red);
        _showConnectivityDialog();
      } else if (e is TimeoutException) {
        _showSnackBar('Connection timeout: Device not responding', Colors.red);
      } else {
        _showSnackBar('Failed to connect: ${e.toString()}', Colors.red);
      }
    }
  }

  // Debug connection with detailed output
  // Removed _testConnection method

  // Scan for device on different IPs
  // Removed _scanForDevice method

  void _connectWebSocket() {
    try {
      webSocketChannel = WebSocketChannel.connect(
        Uri.parse('ws://$deviceIP:81'),
      );

      webSocketSubscription = webSocketChannel!.stream.listen(
        (data) {
          _handleWebSocketMessage(data);
        },
        onError: (error) {
          print('WebSocket error: $error');
          _reconnectWebSocket();
        },
        onDone: () {
          print('WebSocket connection closed');
          _reconnectWebSocket();
        },
      );
    } catch (e) {
      print('WebSocket connection failed: $e');
      _reconnectWebSocket();
    }
  }

  void _handleWebSocketMessage(dynamic data) {
    try {
      final Map<String, dynamic> message = json.decode(data);
      
      if (message['type'] == 'count_update' || message['type'] == 'initial_count') {
        setState(() {
          int newCount = message['count'] ?? 0;
          
          if (message['type'] == 'count_update') {
            if (newCount > currentCount) {
              totalEntered++;
              countHistory.add(CountEvent('Entry', DateTime.now(), newCount));
            } else if (newCount < currentCount) {
              totalExited++;
              countHistory.add(CountEvent('Exit', DateTime.now(), newCount));
            }
          }
          
          currentCount = newCount;
          lastUpdateTime = _formatTime(DateTime.now());
        });
      }
    } catch (e) {
      print('Error parsing WebSocket message: $e');
    }
  }

  void _reconnectWebSocket() {
    _disconnectWebSocket();
    if (isConnected) {
      Timer(const Duration(seconds: 3), () {
        _connectWebSocket();
      });
    }
  }

  void _disconnectWebSocket() {
    webSocketSubscription?.cancel();
    webSocketChannel?.sink.close(status.goingAway);
    webSocketChannel = null;
    webSocketSubscription = null;
  }

  Future<void> _fetchCurrentCount() async {
    try {
      final response = await http.get(
        Uri.parse('http://$deviceIP/api/test'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 5));

      if (response.statusCode == 200) {
        final Map<String, dynamic> data = json.decode(response.body);
        setState(() {
          currentCount = data['count'] ?? 0;
          deviceStatus = data['status'] ?? 'unknown';
          lastUpdateTime = _formatTime(DateTime.now());
        });
      }
    } catch (e) {
      print('Error fetching count: $e');
    }
  }

  Future<void> _fetchDeviceStatus() async {
    try {
      final response = await http.get(
        Uri.parse('http://$deviceIP/api/status'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 5));

      if (response.statusCode == 200) {
        final Map<String, dynamic> data = json.decode(response.body);
        setState(() {
          deviceStatus = data['wifi_connected'] == true ? 'connected' : 'disconnected';
        });
      }
    } catch (e) {
      print('Error fetching status: $e');
    }
  }

  Future<void> _resetCounter() async {
    try {
      final response = await http.post(
        Uri.parse('http://$deviceIP/api/reset'),
        headers: {'Content-Type': 'application/json'},
      ).timeout(const Duration(seconds: 5));

      if (response.statusCode == 200) {
        setState(() {
          currentCount = 0;
          totalEntered = 0;
          totalExited = 0;
          countHistory.clear();
          lastUpdateTime = _formatTime(DateTime.now());
        });
        _showSnackBar('Counter reset successfully!', Colors.green);
      } else {
        _showSnackBar('Failed to reset counter', Colors.red);
      }
    } catch (e) {
      _showSnackBar('Error resetting counter: ${e.toString()}', Colors.red);
    }
  }

  String _formatTime(DateTime time) {
    return '${time.hour.toString().padLeft(2, '0')}:${time.minute.toString().padLeft(2, '0')}:${time.second.toString().padLeft(2, '0')}';
  }

  void _showSnackBar(String message, Color color) {
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(
        content: Text(message),
        backgroundColor: color,
        duration: const Duration(seconds: 4),
      ),
    );
  }

  void _showConnectivityDialog() {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text('Connection Problem'),
          content: SingleChildScrollView(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              mainAxisSize: MainAxisSize.min,
              children: [
                const Text('Cannot connect to ESP8266. Please check:\n'),
                const Text('1. WiFi Connection:', style: TextStyle(fontWeight: FontWeight.bold)),
                const Text('   • Connected to: ESP8266-PeopleCounter'),
                const Text('   • Password: 12345678\n'),
                const Text('2. Android Settings:', style: TextStyle(fontWeight: FontWeight.bold)),
                const Text('   • Don\'t let Android auto-disconnect'),
                const Text('   • Choose "Use network as is" for no internet\n'),
                const Text('3. ESP8266 Status:', style: TextStyle(fontWeight: FontWeight.bold)),
                const Text('   • Check power and LED status'),
                const Text('   • Try restarting the device\n'),
                const Text('4. Network IP:', style: TextStyle(fontWeight: FontWeight.bold)),
                Text('   • Current IP: $deviceIP'),
                Text('   • Try browsing to http://$deviceIP'),
              ],
            ),
          ),
          actions: [
            TextButton(
              child: const Text('Change IP'),
              onPressed: () {
                Navigator.of(context).pop();
                _showSettingsDialog();
              },
            ),
            ElevatedButton(
              child: const Text('Retry'),
              onPressed: () {
                Navigator.of(context).pop();
                _connectToDevice();
              },
            ),
          ],
        );
      },
    );
  }

  void _showSettingsDialog() {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text('Device Settings'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: ipController,
                decoration: const InputDecoration(
                  labelText: 'Device IP Address',
                  hintText: '192.168.4.1',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 16),
              Container(
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.grey[100],
                  borderRadius: BorderRadius.circular(8),
                ),
                child: const Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text('Network Info:', style: TextStyle(fontWeight: FontWeight.bold)),
                    Text('Target: ESP8266-PeopleCounter'),
                    Text('Password: 12345678'),
                    Text('Expected IP: 192.168.4.1'),
                  ],
                ),
              ),
            ],
          ),
          actions: [
            TextButton(
              child: const Text('Cancel'),
              onPressed: () => Navigator.of(context).pop(),
            ),
            TextButton(
              child: const Text('Browser Test'),
              onPressed: () {
                Navigator.of(context).pop();
                _showSnackBar('Open browser: http://${ipController.text}', Colors.blue);
              },
            ),
            ElevatedButton(
              child: const Text('Connect'),
              onPressed: () {
                setState(() {
                  deviceIP = ipController.text;
                  isConnected = false;
                });
                Navigator.of(context).pop();
                _disconnectWebSocket();
                _connectToDevice();
              },
            ),
          ],
        );
      },
    );
  }

  void _showHistoryDialog() {
    showDialog(
      context: context,
      builder: (BuildContext context) {
        return AlertDialog(
          title: const Text('Count History'),
          content: SizedBox(
            width: double.maxFinite,
            height: 300,
            child: countHistory.isEmpty
                ? const Center(child: Text('No history available'))
                : ListView.builder(
                    itemCount: countHistory.length,
                    itemBuilder: (context, index) {
                      final event = countHistory[countHistory.length - 1 - index];
                      return ListTile(
                        leading: Icon(
                          event.type == 'Entry' ? Icons.input : Icons.output,
                          color: event.type == 'Entry' ? Colors.green : Colors.red,
                        ),
                        title: Text(event.type),
                        subtitle: Text(_formatTime(event.timestamp)),
                        trailing: Text('Count: ${event.count}'),
                      );
                    },
                  ),
          ),
          actions: [
            TextButton(
              child: const Text('Clear History'),
              onPressed: () {
                setState(() {
                  countHistory.clear();
                  totalEntered = 0;
                  totalExited = 0;
                });
                Navigator.of(context).pop();
                _showSnackBar('History cleared', Colors.blue);
              },
            ),
            TextButton(
              child: const Text('Close'),
              onPressed: () => Navigator.of(context).pop(),
            ),
          ],
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('People Counter'),
        actions: [
          IconButton(
            icon: const Icon(Icons.history),
            onPressed: _showHistoryDialog,
          ),
          IconButton(
            icon: const Icon(Icons.settings),
            onPressed: _showSettingsDialog,
          ),
        ],
      ),
      body: RefreshIndicator(
        onRefresh: _fetchCurrentCount,
        child: SingleChildScrollView(
          physics: const AlwaysScrollableScrollPhysics(),
          padding: const EdgeInsets.all(16),
          child: Column(
            children: [
              // Connection Status Card
              Card(
                elevation: 4,
                child: Padding(
                  padding: const EdgeInsets.all(16),
                  child: Column(
                    children: [
                      Row(
                        children: [
                          Icon(
                            isConnected ? Icons.wifi : Icons.wifi_off,
                            color: isConnected ? Colors.green : Colors.red,
                            size: 32,
                          ),
                          const SizedBox(width: 12),
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Text(
                                  isConnected ? 'Connected' : isConnecting ? 'Connecting...' : 'Disconnected',
                                  style: TextStyle(
                                    fontSize: 18,
                                    fontWeight: FontWeight.bold,
                                    color: isConnected ? Colors.green : Colors.red,
                                  ),
                                ),
                                Text('Device: $deviceIP'),
                                if (lastUpdateTime.isNotEmpty)
                                  Text('Last update: $lastUpdateTime'),
                              ],
                            ),
                          ),
                        ],
                      ),
                      if (!isConnected && !isConnecting)
                        Padding(
                          padding: const EdgeInsets.only(top: 8),
                          child: Column(
                            children: [
                              Row(
                                children: [
                                  Expanded(
                                    child: ElevatedButton.icon(
                                      onPressed: _connectToDevice,
                                      icon: const Icon(Icons.refresh),
                                      label: const Text('Reconnect'),
                                    ),
                                  ),
                                  // Removed Scan button
                                ],
                              ),
                              // Removed Debug Connection button
                            ],
                          ),
                        ),
                    ],
                  ),
                ),
              ),
              
              const SizedBox(height: 16),
              
              // Current Count Card
              Card(
                elevation: 4,
                child: Padding(
                  padding: const EdgeInsets.all(24),
                  child: Column(
                    children: [
                      Text(
                        'Current Count',
                        style: TextStyle(fontSize: 18, color: Colors.grey[600]),
                      ),
                      const SizedBox(height: 8),
                      Text(
                        currentCount.toString(),
                        style: TextStyle(
                          fontSize: 72,
                          fontWeight: FontWeight.bold,
                          color: Theme.of(context).primaryColor,
                        ),
                      ),
                      Text(
                        'People Inside',
                        style: TextStyle(fontSize: 16, color: Colors.grey[600]),
                      ),
                    ],
                  ),
                ),
              ),
              
              const SizedBox(height: 16),
              
              // Statistics Cards
              Row(
                children: [
                  Expanded(
                    child: Card(
                      elevation: 2,
                      child: Padding(
                        padding: const EdgeInsets.all(16),
                        child: Column(
                          children: [
                            const Icon(Icons.input, color: Colors.green, size: 32),
                            const SizedBox(height: 8),
                            Text(
                              totalEntered.toString(),
                              style: const TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
                            ),
                            const Text('Total Entered'),
                          ],
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: Card(
                      elevation: 2,
                      child: Padding(
                        padding: const EdgeInsets.all(16),
                        child: Column(
                          children: [
                            const Icon(Icons.output, color: Colors.red, size: 32),
                            const SizedBox(height: 8),
                            Text(
                              totalExited.toString(),
                              style: const TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
                            ),
                            const Text('Total Exited'),
                          ],
                        ),
                      ),
                    ),
                  ),
                ],
              ),
              
              const SizedBox(height: 16),
              
              // Control Buttons
              Row(
                children: [
                  Expanded(
                    child: ElevatedButton.icon(
                      onPressed: isConnected ? _resetCounter : null,
                      icon: const Icon(Icons.refresh),
                      label: const Text('Reset Counter'),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: Colors.orange,
                        foregroundColor: Colors.white,
                        padding: const EdgeInsets.symmetric(vertical: 12),
                      ),
                    ),
                  ),
                  const SizedBox(width: 8),
                  Expanded(
                    child: ElevatedButton.icon(
                      onPressed: isConnected ? _fetchCurrentCount : null,
                      icon: const Icon(Icons.sync),
                      label: const Text('Sync Data'),
                      style: ElevatedButton.styleFrom(
                        padding: const EdgeInsets.symmetric(vertical: 12),
                      ),
                    ),
                  ),
                ],
              ),
              
              const SizedBox(height: 16),
              
              // Debug Info Card
              if (!isConnected)
                Card(
                  color: Colors.blue[50],
                  child: Padding(
                    padding: const EdgeInsets.all(12),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        const Text('Troubleshooting:', style: TextStyle(fontWeight: FontWeight.bold)),
                        const Text('1. Connect to ESP8266-PeopleCounter WiFi'),
                        const Text('2. Password: 12345678'),
                        const Text('3. Allow "no internet" connection'),
                        Text('4. Try browser test: http://$deviceIP'),
                        const Text('5. Use "Debug Connection" button'),
                      ],
                    ),
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }
}

class CountEvent {
  final String type;
  final DateTime timestamp;
  final int count;

  CountEvent(this.type, this.timestamp, this.count);
}