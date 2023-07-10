# ESP-CSI Component

[![Component Registry](https://components.espressif.com/components/espressif/esp-csi/badge.svg)](https://components.espressif.com/components/espressif/esp-csi)

- [User Guide](https://github.com/espressif/esp-csi/tree/master/User_Guide.md)

WiFi CSI (Channel State Information) refers to the information obtained from analyzing changes in WiFi signals. This project provides examples of the use of ESP-CSI and the ESP-CSI motion detection algorithm
### Add component to your project

Please use the component manager command `add-dependency` to add the `esp-csi` to your project's dependency, during the `CMake` step the component will be downloaded automatically.

```
idf.py add-dependency "espressif/esp-csi=*"
```

## Example

Please use the component manager command `create-project-from-example` to create the project from example template.

```
idf.py create-project-from-example "espressif/esp-csi=*:csi_recv_router"
```

Then the example will be downloaded in current folder, you can check into it for build and flash.

> You can use this command to download other examples. Or you can download examples from esp-csi repository: 

1. Example of a motion detection algorithm using CSI
   - [connect_rainmaker](https://github.com/espressif/esp-csi/tree/master/examples/connect_rainmaker)
   - [console_test](https://github.com/espressif/esp-csi/tree/master/examples/console_test)

2. Example of outputting raw CSI subcarrier information
   - [csi_recv](https://github.com/espressif/esp-csi/tree/master/examples/get-started/csi_recv)
   - [csi_send](https://github.com/espressif/esp-csi/tree/master/examples/get-started/csi_send)
   - [csi_recv_router](https://github.com/espressif/esp-csi/tree/master/examples/get-started/csi_recv_router)


### Q&A

Q1. I encountered the following problems when using the package manager

```
  HINT: Please check manifest file of the following component(s): main

  ERROR: Because project depends on esp-csi (2.*) which doesn't match any
  versions, version solving failed.
```

A1. For the examples downloaded by using this command, you need to comment out the override_path line in the main/idf_component.yml of each example.

Q2. I encountered the following problems when using the package manager

```
Executing action: create-project-from-example
CMakeLists.txt not found in project directory /home/username
```

A2. This is because an older version packege manager was used, please run `pip install -U idf-component-manager` in ESP-IDF environment to update.
