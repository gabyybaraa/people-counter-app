import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'dart:convert';
import 'dart:async';
import 'dart:io';
import 'network_service.dart';
import 'ui_widgets.dart';

// HTTP Override to allow local network connections
class MyHttpOverrides extends HttpOverrides {
  @override
  HttpClient createHttpClient(SecurityContext? context) {
    return super.createHttpClient(context)
      ..badCertificateCallback =
          (X509Certificate cert, String host, int port) => true;
  }
}

void main() {
  // Allow HTTP requests to local networks
  HttpOverrides.global = MyHttpOverrides();
  runApp(const PeopleCounterApp());
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
        scaffoldBackgroundColor: const Color(0xFFF5F5F5),
      ),
      home: const PeopleCounterHomePage(),
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
  // Controllers and state
  String deviceIP = '192.168.4.1';
  bool isConnected = false;
  bool isConnecting = false;

  // Counter data
  int currentCount = 0;
  String deviceStatus = 'disconnected';
  String lastUpdateTime = '';

  // HTTP Polling Timer
  Timer? pollingTimer;

  // Statistics
  int totalEntered = 0;
  int totalExited = 0;
  List<CountEvent> countHistory = [];

  // Real-time timestamps
  String lastEntryTimeStr = 'Never';
  String lastExitTimeStr = 'Never';
  String currentDeviceTime = '--';

  // Controllers
  final TextEditingController ipController = TextEditingController();
  Timer? reconnectTimer;
  Timer? statusUpdateTimer;

  @override
  void initState() {
    super.initState();
    ipController.text = deviceIP;
    _connectToDevice();

    // HTTP Polling every 2 seconds when connected
    pollingTimer = Timer.periodic(const Duration(seconds: 2), (timer) {
      if (isConnected) {
        _fetchCurrentCount();
      }
    });

    statusUpdateTimer = Timer.periodic(const Duration(seconds: 30), (timer) {
      if (isConnected) {
        _fetchDeviceStatus();
      }
    });
  }

  @override
  void dispose() {
    pollingTimer?.cancel();
    reconnectTimer?.cancel();
    statusUpdateTimer?.cancel();
    ipController.dispose();
    super.dispose();
  }

  // Main connection method - HTTP only
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
        _showSnackBar(
            'Network error: Check WiFi connection to ESP8266-PeopleCounter',
            Colors.red);
        _showConnectivityDialog();
      } else if (e is TimeoutException) {
        _showSnackBar('Connection timeout: Device not responding', Colors.red);
      } else {
        _showSnackBar('Failed to connect: ${e.toString()}', Colors.red);
      }
    }
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
          // Use local time for both device time and last update time since device time is incorrect
          String currentTime = _formatTime(DateTime.now());
          currentDeviceTime = currentTime;
          lastUpdateTime = currentTime;
          
          // Update counter and stats
          int prevCount = currentCount;
          currentCount = data['count'] ?? 0;
          deviceStatus = data['status'] ?? 'unknown';
          
          // Track changes for statistics and update last entry/exit times with correct local time
          if (currentCount > prevCount) {
            int diff = currentCount - prevCount;
            totalEntered += diff;
            // Update last entry time with current local time
            lastEntryTimeStr = currentTime;
            for (int i = 0; i < diff; i++) {
              countHistory.add(CountEvent('Entry', DateTime.now(), currentCount));
            }
          } else if (currentCount < prevCount) {
            int diff = prevCount - currentCount;
            totalExited += diff;
            // Update last exit time with current local time
            lastExitTimeStr = currentTime;
            for (int i = 0; i < diff; i++) {
              countHistory.add(CountEvent('Exit', DateTime.now(), currentCount));
            }
          }
          
          // Only use device timestamps if they're not set yet and no changes detected
          if (lastEntryTimeStr == 'Never' && data.containsKey('last_entry') && data['last_entry'] != 'Never') {
            // Device has a last entry but we don't - this means it happened before app started
            lastEntryTimeStr = 'Before app start';
          }
          
          if (lastExitTimeStr == 'Never' && data.containsKey('last_exit') && data['last_exit'] != 'Never') {
            // Device has a last exit but we don't - this means it happened before app started  
            lastExitTimeStr = 'Before app start';
          }
        });
      }
    } catch (e) {
      print('Error fetching count: $e');
      // If we can't fetch data, assume disconnected
      if (isConnected) {
        setState(() {
          isConnected = false;
        });
        _showSnackBar('Connection lost', Colors.red);
      }
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
          deviceStatus =
              data['wifi_connected'] == true ? 'connected' : 'disconnected';
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
          lastEntryTimeStr = 'Never';
          lastExitTimeStr = 'Never';
          lastUpdateTime = _formatTime(DateTime.now());
          currentDeviceTime = lastUpdateTime;
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
                const Text('1. WiFi Connection:',
                    style: TextStyle(fontWeight: FontWeight.bold)),
                const Text('   • Connected to: ESP8266-PeopleCounter'),
                const Text('   • Password: 12345678\n'),
                const Text('2. Android Settings:',
                    style: TextStyle(fontWeight: FontWeight.bold)),
                const Text('   • Don\'t let Android auto-disconnect'),
                const Text('   • Choose "Use network as is" for no internet\n'),
                const Text('3. ESP8266 Status:',
                    style: TextStyle(fontWeight: FontWeight.bold)),
                const Text('   • Check power and LED status'),
                const Text('   • Try restarting the device\n'),
                const Text('4. Network IP:',
                    style: TextStyle(fontWeight: FontWeight.bold)),
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
                    Text('Network Info:',
                        style: TextStyle(fontWeight: FontWeight.bold)),
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
                _showSnackBar(
                    'Open browser: http://${ipController.text}', Colors.blue);
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
                      final event =
                          countHistory[countHistory.length - 1 - index];
                      return ListTile(
                        leading: Icon(
                          event.type == 'Entry' ? Icons.input : Icons.output,
                          color:
                              event.type == 'Entry' ? Colors.green : Colors.red,
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
      backgroundColor: const Color(0xFFF5F5F5),
      appBar: AppBar(
        title: const Text(
          'People Counter',
          style: TextStyle(
            fontSize: 24,
            fontWeight: FontWeight.bold,
            color: Colors.black,
          ),
        ),
        backgroundColor: Colors.white,
        elevation: 1,
        actions: [
          IconButton(
            icon: const Icon(Icons.history, color: Colors.black54),
            onPressed: _showHistoryDialog,
          ),
          IconButton(
            icon: const Icon(Icons.settings, color: Colors.black54),
            onPressed: _showSettingsDialog,
          ),
        ],
      ),
      body: RefreshIndicator(
        onRefresh: _fetchCurrentCount,
        child: SingleChildScrollView(
          physics: const AlwaysScrollableScrollPhysics(),
          padding: const EdgeInsets.all(20),
          child: Column(
            children: [
              // Connection Status Card
              buildCard(
                child: Row(
                  children: [
                    Icon(
                      Icons.wifi,
                      color: isConnected ? Colors.green : Colors.red,
                      size: 28,
                    ),
                    const SizedBox(width: 16),
                    Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            isConnected
                                ? 'Connected'
                                : isConnecting
                                    ? 'Connecting...'
                                    : 'Disconnected',
                            style: TextStyle(
                              fontSize: 20,
                              fontWeight: FontWeight.bold,
                              color: isConnected ? Colors.green : Colors.red,
                            ),
                          ),
                          const SizedBox(height: 4),
                          Text(
                            'Device: ${isConnected ? deviceIP : '--'}',
                            style: const TextStyle(
                              color: Colors.black87,
                              fontSize: 16,
                              fontWeight: FontWeight.w500,
                            ),
                          ),
                          if (lastUpdateTime.isNotEmpty)
                            Text(
                              'Last update: $lastUpdateTime',
                              style: const TextStyle(
                                color: Colors.black87,
                                fontSize: 16,
                                fontWeight: FontWeight.w500,
                              ),
                            ),
                        ],
                      ),
                    ),
                    if (!isConnected && !isConnecting)
                      TextButton(
                        onPressed: _connectToDevice,
                        child: const Text('Reconnect'),
                      ),
                  ],
                ),
              ),

              const SizedBox(height: 24),

              // Current Count Card
              buildCard(
                child: Column(
                  children: [
                    const Text(
                      'Current Count',
                      style: TextStyle(
                        fontSize: 18,
                        color: Colors.grey,
                        fontWeight: FontWeight.w500,
                      ),
                    ),
                    const SizedBox(height: 24),
                    Text(
                      currentCount.toString(),
                      style: const TextStyle(
                        fontSize: 120,
                        fontWeight: FontWeight.bold,
                        color: Color(0xFF6366F1), // Purple color like in image
                      ),
                    ),
                    const SizedBox(height: 16),
                    const Text(
                      'People Inside',
                      style: TextStyle(
                        fontSize: 18,
                        color: Colors.grey,
                        fontWeight: FontWeight.w500,
                      ),
                    ),
                  ],
                ),
              ),

              const SizedBox(height: 24),

              // Activity & Timing Card
              buildCard(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Row(
                      children: [
                        Icon(
                          Icons.access_time,
                          color: Colors.black54,
                          size: 24,
                        ),
                        SizedBox(width: 12),
                        Text(
                          'Activity & Timing',
                          style: TextStyle(
                            fontSize: 20,
                            fontWeight: FontWeight.bold,
                            color: Colors.black87,
                          ),
                        ),
                      ],
                    ),
                    const SizedBox(height: 20),
                    
                    // Current Time
                    Container(
                      padding: const EdgeInsets.all(16),
                      decoration: BoxDecoration(
                        color: Colors.grey[50],
                        borderRadius: BorderRadius.circular(12),
                        border: Border.all(color: Colors.grey[300]!),
                      ),
                      child: Row(
                        children: [
                          Container(
                            padding: const EdgeInsets.all(6),
                            decoration: BoxDecoration(
                              color: Colors.grey[200],
                              borderRadius: BorderRadius.circular(8),
                            ),
                            child: const Icon(
                              Icons.access_time,
                              color: Colors.black54,
                              size: 16,
                            ),
                          ),
                          const SizedBox(width: 12),
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                const Text(
                                  'Current Time',
                                  style: TextStyle(
                                    color: Colors.grey,
                                    fontSize: 12,
                                    fontWeight: FontWeight.w500,
                                  ),
                                ),
                                Text(
                                  currentDeviceTime,
                                  style: const TextStyle(
                                    color: Colors.black87,
                                    fontSize: 16,
                                    fontWeight: FontWeight.w600,
                                  ),
                                ),
                              ],
                            ),
                          ),
                        ],
                      ),
                    ),
                    
                    const SizedBox(height: 12),
                    
                    // Last Entry
                    Container(
                      padding: const EdgeInsets.all(16),
                      decoration: BoxDecoration(
                        color: Colors.grey[50],
                        borderRadius: BorderRadius.circular(12),
                        border: Border.all(color: Colors.grey[300]!),
                      ),
                      child: Row(
                        children: [
                          Container(
                            padding: const EdgeInsets.all(6),
                            decoration: BoxDecoration(
                              color: Colors.green[50],
                              borderRadius: BorderRadius.circular(8),
                            ),
                            child: const Icon(
                              Icons.login,
                              color: Colors.green,
                              size: 16,
                            ),
                          ),
                          const SizedBox(width: 12),
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                const Text(
                                  'Last Entry',
                                  style: TextStyle(
                                    color: Colors.grey,
                                    fontSize: 12,
                                    fontWeight: FontWeight.w500,
                                  ),
                                ),
                                Text(
                                  lastEntryTimeStr,
                                  style: const TextStyle(
                                    color: Colors.black87,
                                    fontSize: 16,
                                    fontWeight: FontWeight.w600,
                                  ),
                                ),
                              ],
                            ),
                          ),
                          if (lastEntryTimeStr != 'Never' && lastEntryTimeStr != 'Before app start')
                            Container(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 8,
                                vertical: 4,
                              ),
                              decoration: BoxDecoration(
                                color: Colors.green[100],
                                borderRadius: BorderRadius.circular(12),
                              ),
                              child: const Text(
                                'IN',
                                style: TextStyle(
                                  color: Colors.green,
                                  fontSize: 10,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                            ),
                        ],
                      ),
                    ),
                    
                    const SizedBox(height: 12),
                    
                    // Last Exit
                    Container(
                      padding: const EdgeInsets.all(16),
                      decoration: BoxDecoration(
                        color: Colors.grey[50],
                        borderRadius: BorderRadius.circular(12),
                        border: Border.all(color: Colors.grey[300]!),
                      ),
                      child: Row(
                        children: [
                          Container(
                            padding: const EdgeInsets.all(6),
                            decoration: BoxDecoration(
                              color: Colors.red[50],
                              borderRadius: BorderRadius.circular(8),
                            ),
                            child: const Icon(
                              Icons.logout,
                              color: Colors.red,
                              size: 16,
                            ),
                          ),
                          const SizedBox(width: 12),
                          Expanded(
                            child: Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                const Text(
                                  'Last Exit',
                                  style: TextStyle(
                                    color: Colors.grey,
                                    fontSize: 12,
                                    fontWeight: FontWeight.w500,
                                  ),
                                ),
                                Text(
                                  lastExitTimeStr,
                                  style: const TextStyle(
                                    color: Colors.black87,
                                    fontSize: 16,
                                    fontWeight: FontWeight.w600,
                                  ),
                                ),
                              ],
                            ),
                          ),
                          if (lastExitTimeStr != 'Never' && lastExitTimeStr != 'Before app start')
                            Container(
                              padding: const EdgeInsets.symmetric(
                                horizontal: 8,
                                vertical: 4,
                              ),
                              decoration: BoxDecoration(
                                color: Colors.red[100],
                                borderRadius: BorderRadius.circular(12),
                              ),
                              child: const Text(
                                'OUT',
                                style: TextStyle(
                                  color: Colors.red,
                                  fontSize: 10,
                                  fontWeight: FontWeight.bold,
                                ),
                              ),
                            ),
                        ],
                      ),
                    ),
                  ],
                ),
              ),

              const SizedBox(height: 24),

              // Statistics Cards Row
              Row(
                children: [
                  // Total Entered Card
                  Expanded(
                    child: buildCard(
                      child: Column(
                        children: [
                          Container(
                            padding: const EdgeInsets.all(12),
                            decoration: BoxDecoration(
                              color: Colors.green[50],
                              borderRadius: BorderRadius.circular(12),
                            ),
                            child: const Icon(
                              Icons.login,
                              color: Colors.green,
                              size: 32,
                            ),
                          ),
                          const SizedBox(height: 16),
                          Text(
                            totalEntered.toString(),
                            style: const TextStyle(
                              fontSize: 48,
                              fontWeight: FontWeight.bold,
                              color: Colors.black87,
                            ),
                          ),
                          const SizedBox(height: 8),
                          const Text(
                            'Total Entered',
                            style: TextStyle(
                              fontSize: 16,
                              color: Colors.black87,
                              fontWeight: FontWeight.w600,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                  
                  const SizedBox(width: 16),
                  
                  // Total Exited Card
                  Expanded(
                    child: buildCard(
                      child: Column(
                        children: [
                          Container(
                            padding: const EdgeInsets.all(12),
                            decoration: BoxDecoration(
                              color: Colors.red[50],
                              borderRadius: BorderRadius.circular(12),
                            ),
                            child: const Icon(
                              Icons.logout,
                              color: Colors.red,
                              size: 32,
                            ),
                          ),
                          const SizedBox(height: 16),
                          Text(
                            totalExited.toString(),
                            style: const TextStyle(
                              fontSize: 48,
                              fontWeight: FontWeight.bold,
                              color: Colors.black87,
                            ),
                          ),
                          const SizedBox(height: 8),
                          const Text(
                            'Total Exited',
                            style: TextStyle(
                              fontSize: 16,
                              color: Colors.black87,
                              fontWeight: FontWeight.w600,
                            ),
                          ),
                        ],
                      ),
                    ),
                  ),
                ],
              ),

              const SizedBox(height: 32),

              // Control Buttons
              Row(
                children: [
                  Expanded(
                    child: Container(
                      height: 56,
                      decoration: BoxDecoration(
                        borderRadius: BorderRadius.circular(28),
                        gradient: const LinearGradient(
                          colors: [Color(0xFFFF8A00), Color(0xFFFF6B00)],
                        ),
                        boxShadow: [
                          BoxShadow(
                            color: Colors.orange.withOpacity(0.3),
                            blurRadius: 12,
                            offset: const Offset(0, 4),
                          ),
                        ],
                      ),
                      child: ElevatedButton.icon(
                        onPressed: isConnected ? _resetCounter : null,
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.transparent,
                          shadowColor: Colors.transparent,
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(28),
                          ),
                        ),
                        icon: const Icon(
                          Icons.refresh,
                          color: Colors.white,
                          size: 20,
                        ),
                        label: const Text(
                          'Reset Counter',
                          style: TextStyle(
                            color: Colors.white,
                            fontSize: 16,
                            fontWeight: FontWeight.w600,
                          ),
                        ),
                      ),
                    ),
                  ),
                  const SizedBox(width: 16),
                  Expanded(
                    child: Container(
                      height: 56,
                      decoration: BoxDecoration(
                        color: Colors.white,
                        borderRadius: BorderRadius.circular(28),
                        border: Border.all(color: Colors.grey.shade300),
                        boxShadow: [
                          BoxShadow(
                            color: Colors.black.withOpacity(0.05),
                            blurRadius: 12,
                            offset: const Offset(0, 4),
                          ),
                        ],
                      ),
                      child: ElevatedButton.icon(
                        onPressed: isConnected ? _fetchCurrentCount : null,
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.transparent,
                          shadowColor: Colors.transparent,
                          shape: RoundedRectangleBorder(
                            borderRadius: BorderRadius.circular(28),
                          ),
                        ),
                        icon: Icon(
                          Icons.sync,
                          color: isConnected ? const Color(0xFF6366F1) : Colors.grey,
                          size: 20,
                        ),
                        label: Text(
                          'Sync Data',
                          style: TextStyle(
                            color: isConnected ? const Color(0xFF6366F1) : Colors.grey,
                            fontSize: 16,
                            fontWeight: FontWeight.w600,
                          ),
                        ),
                      ),
                    ),
                  ),
                ],
              ),

              const SizedBox(height: 20),

              // Debug Info Card
              if (!isConnected)
                buildCard(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      const Text(
                        'Troubleshooting:',
                        style: TextStyle(
                          fontWeight: FontWeight.bold,
                          color: Colors.blue,
                          fontSize: 16,
                        ),
                      ),
                      const SizedBox(height: 12),
                      const Text('1. Connect to ESP8266-PeopleCounter WiFi'),
                      const Text('2. Password: 12345678'),
                      const Text('3. Allow "no internet" connection'),
                      Text('4. Try browser test: http://$deviceIP'),
                      const Text('5. Check device power and status'),
                    ],
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