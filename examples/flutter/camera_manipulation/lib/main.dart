import 'dart:async';
import 'package:logging/logging.dart';
import 'package:flutter/material.dart';
import 'package:thermion_flutter/thermion_flutter.dart';
import 'package:vector_math/vector_math_64.dart' show Vector3, makeViewMatrix;

void main() async {
  runApp(const MyApp());
  Logger.root.onRecord.listen((record) {
    print(record);
  });
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Thermion Demo',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
        useMaterial3: true,
      ),
      home: const MyHomePage(title: 'Thermion Demo Home Page'),
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});
  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  late DelegateInputHandler _fixedOrbitInputHandler;
  late DelegateInputHandler _freeFlightInputHandler;
  final mininumDistance = -7.0;

  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback((_) async {
      _thermionViewer = await ThermionFlutterPlugin.createViewer();
      var entity =
          await _thermionViewer!.loadGlb("assets/test_1.glb", keepData: true);
      await _thermionViewer!.transformToUnitCube(entity);
      await _thermionViewer!.loadSkybox("assets/default_env_skybox.ktx");
      await _thermionViewer!.loadIbl("assets/default_env_ibl.ktx");
      // await _thermionViewer!.setBackgroundColor;

      await _thermionViewer!.setPostProcessing(true);
      await _thermionViewer!.setRendering(true);

      _fixedOrbitInputHandler =
          DelegateInputHandler.fixedOrbit(_thermionViewer!)
            ..setActionForType(InputType.MMB_HOLD_AND_MOVE, InputAction.ROTATE)
            ..setActionForType(InputType.SCALE1, InputAction.ROTATE)
            ..setActionForType(InputType.SCALE2, InputAction.ZOOM)
            ..setActionForType(InputType.SCROLLWHEEL, InputAction.ZOOM);

      _freeFlightInputHandler = DelegateInputHandler.flight(_thermionViewer!)
        ..setActionForType(InputType.MMB_HOLD_AND_MOVE, InputAction.ROTATE)
        ..setActionForType(InputType.SCALE1, InputAction.ROTATE)
        ..setActionForType(InputType.SCALE2, InputAction.ZOOM)
        ..setActionForType(InputType.SCROLLWHEEL, InputAction.ZOOM);

      setState(() {});
    });
  }

  ThermionViewer? _thermionViewer;

  bool isOrbit = true;
  void reset() async {
    var initialPosition = Vector3(0, 0, -mininumDistance);
    var lookAt = Vector3.zero();
    var up = Vector3(0, 1.0, 0);
    var viewMatrix = makeViewMatrix(initialPosition, lookAt, up);
    viewMatrix.invert();
    await _thermionViewer!.setCameraModelMatrix4(viewMatrix);
    setState(() {});
  }

  @override
  Widget build(BuildContext context) {
    return Stack(children: [
      if (_thermionViewer != null) ...[
        Positioned.fill(
          child: ThermionListenerWidget(
            inputHandler:
                isOrbit ? _fixedOrbitInputHandler : _freeFlightInputHandler,
            child: ThermionWidget(
              viewer: _thermionViewer!,
            ),
          ),
        ),
        Column(
          mainAxisAlignment: MainAxisAlignment.end,
          children: [
            ElevatedButton(
                onPressed: () {
                  isOrbit = !isOrbit;
                  setState(() {});
                },
                child: Text("Switch to ${isOrbit ? "Free Flight" : "Orbit"}")),
            ElevatedButton(
              onPressed: () => reset(),
              child: Text("Reset"),
            )
          ],
        )
      ],
    ]);
  }
}
