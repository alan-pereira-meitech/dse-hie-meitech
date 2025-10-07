# Automation-API


[![Build status](https://hbmdevelopment.visualstudio.com/HBM%20Weighing/_apis/build/status/HBM%20Weighing%20API%20CI)](https://hbmdevelopment.visualstudio.com/HBM%20Weighing/_build/latest?definitionId=47)

Connect your own application to weighing terminals WTX110 and WTX120 or digital sensor electronic DSE from HBM.


Contains API and 3 templates (Console application, Simple GUI, PLC view).

## C++ Port for Linux

The original .NET API has been ported to modern C++ and CMake in the [`cpp/`](cpp/) directory.
The new implementation targets Ubuntu and models the JetBus communication flow using
reference implementations from [node-jet](https://github.com/HBM/node-jet) and [SharpJet](https://github.com/HBM/SharpJet/).

### Build

```bash
cd cpp
cmake -S . -B build
cmake --build build
```

### Test

```bash
cd cpp
cmake --build build --target test_wtx
ctest --test-dir build
```

### Example

After building, run the example console application to see the simulated JetBus polling loop in action:

```bash
./build/wtx_example
```

Documentation can be found on the [product page](https://www.hbm.com/wtx/).

## License

Copyright (c) 2019 HBM. See the [LICENSE](LICENSE) file for license rights and
limitations (MIT).
