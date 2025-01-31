// MIT License
//
// Copyright (c) 2020 RomanSo
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cassert>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

#include "dbus_gatt/dbus_gatt.h"
#include "dbus_gatt/dbus_gatt_exceptions.h"
#include "future"

static constexpr auto kTestCharValueOnRead{1000};
static constexpr auto kTestReadOnlyValue{1234};
static constexpr auto kSetTestCharacteristicValueTimeOut{5};

class ExampleDBusGATT: public dbus_gatt::DBusGATT {
public:
    ExampleDBusGATT(): dbus_gatt::DBusGATT("dbus_gatt.example", "/dbus_gatt/example") {
        Services(dbus_gatt::DBusGATT::Service(
            "device",
            "0000180a-0000-1000-8000-00805f9b34fb",
            dbus_gatt::DBusGATT::Characteristic(
                "mfgr_name",
                "00002a29-0000-1000-8000-00805f9b34fb",
                dbus_gatt::kCharacteristicFlagRead,
                []() -> dbus_gatt::DBusGattVariantT {
                    std::cout << "()()" << std::endl;
                    return std::string("hello");
                },
                nullptr),
            dbus_gatt::DBusGATT::Characteristic(
                "test_char",
                "00002a30-0000-1000-8000-00805f9b34fb",
                static_cast<dbus_gatt::CharacteristicFlag>(dbus_gatt::kCharacteristicFlagRead |
                                                           dbus_gatt::kCharacteristicFlagWrite) |
                    dbus_gatt::kCharacteristicFlagNotify,

                []() -> dbus_gatt::DBusGattVariantT {
                    return static_cast<int32_t>(kTestCharValueOnRead);
                },
                [](const uint8_t* value, size_t size) {
                    std::cout << "write size " << size << std::endl;

                    if(value == nullptr) {
                        std::cout << "Wrong ptr" << std::endl;
                    }

                    std::cout << static_cast<const unsigned char*>(value) << std::endl;
                    return static_cast<int32_t>(0);
                }),
            dbus_gatt::DBusGATT::ReadOnlyValueCharacteristic(
                "test_read_only_value",
                "00002a31-0000-1000-8000-00805f9b34fb",
                dbus_gatt::kCharacteristicFlagRead,
                dbus_gatt::DBusGattVariantT(static_cast<int32_t>(kTestReadOnlyValue)))));

        addDevicePropertyChangedCallback(dbus_gatt::EDeviceProperty::Connected,
                                         [self = this](const dbus_gatt::DBusGattVariantT& value) {
                                             try {
                                                 if(std::get<bool>(value)) {
                                                     deviceConnected();
                                                 } else {
                                                     deviceDisconnected();
                                                 }
                                             } catch(std::bad_variant_access&) {
                                                 std::cout << "unsupported property value"
                                                           << std::endl;
                                             }
                                         });
        notify_handle_ = std::async(std::launch::async, [&]() { notify(); });
    }
private:
    std::future<void> notify_handle_;
    void notify() {
        int i = 0;
        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(kSetTestCharacteristicValueTimeOut));
            setCharacteristicValue(std::string("xo-xo-xo_") + std::to_string(i++),
                                   "/dbus_gatt/example/device/test_char",
                                   false);
        }
    }

    static void deviceConnected() {
        std::cout << "device connected" << std::endl;
    }

    static void deviceDisconnected() {
        std::cout << "device disconnected" << std::endl;
    }
};

int main() {
    try {
        auto app = ExampleDBusGATT();
        app.start();

    } catch(DBusGATTException& e) {
        std::cout << "FATAL ERROR: " << e.what() << std::endl;
        return 0;
    }
    return 0;
}
