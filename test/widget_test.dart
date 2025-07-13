// This is a basic Flutter widget test.
//
// To perform an interaction with a widget in your test, use the WidgetTester
// utility in the flutter_test package. For example, you can send tap and scroll
// gestures. You can also use WidgetTester to find child widgets in the widget
// tree, read text, and verify that the values of widget properties are correct.

import 'package:flutter_test/flutter_test.dart';

import 'package:people_counter/main.dart';

void main() {
  testWidgets('People counter app smoke test', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(PeopleCounterApp());

    // Verify that the app title is displayed
    expect(find.text('People Counter'), findsOneWidget);
    
    // Verify that we have multiple zeros (main count + statistics)
    expect(find.text('0'), findsNWidgets(3));
    
    // Verify that connection status elements exist
    expect(find.text('Current Count'), findsOneWidget);
    expect(find.text('People Inside'), findsOneWidget);
    expect(find.text('Total Entered'), findsOneWidget);
    expect(find.text('Total Exited'), findsOneWidget);
    
    // Verify that control buttons exist
    expect(find.text('Reset Counter'), findsOneWidget);
    expect(find.text('Sync Data'), findsOneWidget);
  });
}