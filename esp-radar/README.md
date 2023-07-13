# esp-radar Component

[![Component Registry](https://components.espressif.com/components/espressif/esp-radar/badge.svg)](https://components.espressif.com/components/espressif/esp-radar)

- [User Guide](https://github.com/espressif/esp-csi/tree/master/README.md)

WiFi CSI (Channel State Information) refers to the information obtained from analyzing changes in WiFi signals. This project provides examples of the ESP-CSI motion detection algorithm

### Add component to your project
Please use the component manager command `add-dependency` to add the `esp-radar` to your project's dependency, during the `CMake` step the component will be downloaded automatically.

```
idf.py add-dependency "espressif/esp-radar=*"
```

## Example
Please use the component manager command `create-project-from-example` to create the project from example template.

```
idf.py create-project-from-example "espressif/esp-radar=*:console_test"
```

Then the example will be downloaded in current folder, you can check into it for build and flash.

> You can use this command to download other examples. Or you can download examples from esp-radar repository: 

 - [connect_rainmaker](https://github.com/espressif/esp-radar/tree/master/examples/connect_rainmaker): Adding Wi-Fi CSI Functionality in ESP RainMaker
 - [console_test](https://github.com/espressif/esp-radar/tree/master/examples/console_test): This example provides a test platform for Wi-Fi CSI, which includes functions such as data display, data acquisition and data analysis, which can help you quickly understand Wi-Fi CSI

### Q&A
Q1. I encountered the following problems when using the package manager

```
  HINT: Please check manifest file of the following component(s): main

  ERROR: Because project depends on esp-radar (2.*) which doesn't match any
  versions, version solving failed.
```

A1. For the examples downloaded by using this command, you need to comment out the override_path line in the main/idf_component.yml of each example.

Q2. I encountered the following problems when using the package manager

```
Executing action: create-project-from-example
CMakeLists.txt not found in project directory /home/username
```

A2. This is because an older version packege manager was used, please run `pip install -U idf-component-manager` in ESP-IDF environment to update.
