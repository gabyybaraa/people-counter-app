import 'package:http/http.dart' as http;
import 'dart:convert';
import 'dart:async';

class NetworkService {
  static Future<Map<String, dynamic>> connectToDevice(String deviceIP) async {
    final response = await http.get(
      Uri.parse('http://$deviceIP/api/test'),
      headers: {'Content-Type': 'application/json'},
    ).timeout(const Duration(seconds: 10));
    if (response.statusCode == 200) {
      return json.decode(response.body);
    } else {
      throw Exception('Device responded with status: ${response.statusCode}');
    }
  }

  static Future<Map<String, dynamic>> fetchCurrentCount(String deviceIP) async {
    final response = await http.get(
      Uri.parse('http://$deviceIP/api/test'),
      headers: {'Content-Type': 'application/json'},
    ).timeout(const Duration(seconds: 5));
    if (response.statusCode == 200) {
      return json.decode(response.body);
    } else {
      throw Exception('Device responded with status: ${response.statusCode}');
    }
  }

  static Future<Map<String, dynamic>> fetchDeviceStatus(String deviceIP) async {
    final response = await http.get(
      Uri.parse('http://$deviceIP/api/status'),
      headers: {'Content-Type': 'application/json'},
    ).timeout(const Duration(seconds: 5));
    if (response.statusCode == 200) {
      return json.decode(response.body);
    } else {
      throw Exception('Device responded with status: ${response.statusCode}');
    }
  }

  static Future<Map<String, dynamic>> resetCounter(String deviceIP) async {
    final response = await http.post(
      Uri.parse('http://$deviceIP/api/reset'),
      headers: {'Content-Type': 'application/json'},
    ).timeout(const Duration(seconds: 5));
    if (response.statusCode == 200) {
      return json.decode(response.body);
    } else {
      throw Exception('Device responded with status: ${response.statusCode}');
    }
  }
} 