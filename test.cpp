#include <FredEmmott/USBIP.hpp>
#include <FredEmmott/USBSpec.hpp>

#include <print>

#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

namespace USB = FredEmmott::USBSpec;
namespace USBIP = FredEmmott::USBIP;
using USBIP::ByteOrder::to_network;
using USBIP::ByteOrder::from_network;

namespace USBStrings {
    enum class Indices : uint8_t {
        LangID = 0,
        Manufacturer,
        Product,
        SerialNumber,
        Interface,
    };

    constexpr auto LangID = USB::StringDescriptor{L"\x0409"}; // en_US
    constexpr auto Manufacturer = USB::StringDescriptor{L"Fred"};
    constexpr auto Product = USB::StringDescriptor{L"FakeMouse"};
    constexpr auto SerialNumber = USB::StringDescriptor{L"6969"};
    constexpr auto Interface = USB::StringDescriptor{L"FakeInterface"};
};

// HID Report Descriptor (Minimal Boot Mouse: Buttons (1 byte) + X (1 byte) + Y (1 byte))
const uint8_t ReportDescriptor[] = {
    0x05, 0x01, // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02, // Usage (Mouse)
    0xA1, 0x01, // Collection (Application)
    0x09, 0x01, //   Usage (Pointer)
    0xA1, 0x00, //   Collection (Physical)
    0x05, 0x09, //     Usage Page (Button)
    0x19, 0x01, //     Usage Minimum (0x01)
    0x29, 0x03, //     Usage Maximum (0x03)
    0x15, 0x00, //     Logical Minimum (0)
    0x25, 0x01, //     Logical Maximum (1)
    0x95, 0x03, //     Report Count (3)
    0x75, 0x01, //     Report Size (1)
    0x81, 0x02, //     Report Input (Data, Variable, Absolute)
    0x95, 0x05, //     Report Count (5)
    0x81, 0x03, //     Report Input (Constant, Variable, Absolute) ; padding
    0x05, 0x01, //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30, //     Usage (X)
    0x09, 0x31, //     Usage (Y)
    0x15, 0x81, //     Logical Minimum (-127)
    0x25, 0x7F, //     Logical Maximum (127)
    0x75, 0x08, //     Report Size (8)
    0x95, 0x02, //     Report Count (2)
    0x81, 0x06, //     Report Input (Data, Variable, Relative)
    0xC0, //   End Collection
    0xC0 // End Collection
};


bool send_all(SOCKET sock, const void *buffer, const size_t len) {
    const char *ptr = (const char *) buffer;
    int sent = 0;
    while (sent < len) {
        int result = send(sock, ptr + sent, (int) (len - sent), 0);
        if (result == SOCKET_ERROR) {
            std::println(stderr, "Send failed: {}", WSAGetLastError());
            return false;
        }
        sent += result;
    }
    return true;
}

template<class T>
bool send_all(SOCKET sock, const T &what) {
    return send_all(sock, &what, sizeof(T));
}

bool recv_all(SOCKET sock, void *buffer, size_t len) {
    char *ptr = (char *) buffer;
    int received = 0;
    while (received < len) {
        int result = recv(sock, ptr + received, (int) (len - received), 0);
        if (result == 0) {
            std::println(stderr, "Connection closed by peer.");
            return false;
        }
        if (result == SOCKET_ERROR) {
            std::println(stderr, "Recv failed: {}", WSAGetLastError());
            return false;
        }
        received += result;
    }
    return true;
}

template<class T>
bool recv_all(SOCKET sock, T *what) {
    return recv_all(sock, what, sizeof(std::decay_t<T>));
}

constexpr USBIP::Device MouseDevice{
    .mPath = "/fake-device",
    .mBusID = "1-1",
    .mBusNum = to_network<uint32_t>(1),
    .mDevNum = to_network<uint32_t>(1),
    .mSpeed = USBIP::Speed::Full,
    .mVendorID = 0x4242,
    .mProductID = 0x4242,
    .mDeviceVersion = to_network<uint16_t>(0x01'00),
    .mConfigurationValue = 1,
    .mNumConfigurations = 1,
    .mNumInterfaces = 1,
};
constexpr USB::DeviceDescriptor MouseDeviceDescriptor{
    .mDescriptorType = 0x01, // DEVICE
    .mUSBVersion = 0x02'00,
    .mMaxPacketSize0 = 0x40,
    .mVendorID = from_network(MouseDevice.mVendorID),
    .mProductID = from_network(MouseDevice.mProductID),
    .mDeviceVersion = from_network(MouseDevice.mDeviceVersion),
    .mManufacturerStringIndex = std::to_underlying(USBStrings::Indices::Manufacturer),
    .mProductStringIndex = std::to_underlying(USBStrings::Indices::Product),
    .mSerialNumberStringIndex = std::to_underlying(USBStrings::Indices::SerialNumber),
    .mNumConfigurations = MouseDevice.mNumConfigurations,
};

constexpr USBIP::Interface MouseInterface{
    .mClass = 3, // HID
    .mSubClass = 1, // Boot interface
    .mProtocol = 2, // Mouse
};

constexpr struct MouseConfigurationDescriptor {
    USB::ConfigurationDescriptor mConfiguration{
        .mTotalLength = sizeof(MouseConfigurationDescriptor),
        .mNumInterfaces = 1,
        .mConfigurationValue = 1,
        .mConfiguration = 0,
        .mAttributes = 0x80 | 0x20, // bus-powered, remote wakeup
        .mMaxPower = 0x32, // 100ma
    };
    USB::InterfaceDescriptor mInterface{
        .mNumEndpoints = 1,
        .mClass = MouseInterface.mClass,
        .mSubClass = MouseInterface.mSubClass,
        .mProtocol = MouseInterface.mProtocol,
        .mInterfaceStringIndex = std::to_underlying(USBStrings::Indices::Interface),
    };
    USB::HIDDescriptor mHID{
        .mLength = sizeof(USB::HIDDescriptor) + sizeof(USB::HIDDescriptor::Report),
        .mHIDSpecVersion = 0x01'11,
        .mNumDescriptors = 1,
    };
    USB::HIDDescriptor::Report mHIDReportDescriptor{
        .mType = 0x22, // HID Report
        .mLength = sizeof(ReportDescriptor),
    };
    USB::EndpointDescriptor mEndpoint{
        .mEndpointAddress = 0x80 | 0x01, // In, endpoint 1
        .mAttributes = 0x03, // interrupt
        .mMaxPacketSize = 0x00'04,
        .mInterval = 0x0a, // 10ms polling
    };
} MouseConfigurationDescriptor;

bool handle_devlist_request(SOCKET sock) {
    std::println("-> Received OP_REQ_DEVLIST. Sending OP_REP_DEVLIST.");
    const USBIP::OP_REP_DEVLIST header{
        .mNumDevices = to_network<uint32_t>(1),
    };

    if (!send_all(sock, header)) return false;
    if (!send_all(sock, MouseDevice)) return false;
    if (!send_all(sock, MouseInterface)) return false;

    std::println("-> DEVLIST sent successfully.");
    return true;
}

bool handle_import_request(SOCKET sock, const char *busid) {
    std::println("-> Received OP_REQ_IMPORT for {}. Sending OP_REP_IMPORT.", busid);

    if (!send_all(sock, USBIP::OP_REP_IMPORT{.mDevice = MouseDevice})) return false;

    std::println("-> IMPORT sent successfully. Device is now virtualized.");
    return true;
}

bool handle_submit_request(SOCKET sock, const USBIP::USBIP_CMD_SUBMIT &command) {
    const auto endpoint = from_network(command.mHeader.mEndpoint);
    const auto transferBufferLength = from_network(command.mTransferBufferLength);

    if (endpoint == 0) {
        const auto &s = command.mSetup;

        std::println("-> CMD_SUBMIT (Control EP0, Seq: {}, Type: 0x{:x}, Req: 0x{:x}, Len: {})",
                     from_network(command.mHeader.mSequenceNumber),
                     (int) s.mRequestType,
                     (int) s.mRequest,
                     s.mLength);

        const void *responseData = nullptr;
        size_t responseDataSize = 0;

        constexpr uint8_t HostToDevice = 0x00;
        constexpr uint8_t DeviceToHost = 0x80;
        constexpr uint8_t DeviceToInterface = 0x81;

        if (s.mRequestType == DeviceToHost && s.mRequest == 0x06) {
            const auto descriptorType = (uint8_t) (s.mValue >> 8);
            const uint8_t descriptorIndex = (uint8_t) (s.mValue & 0xFF);
            std::println("Getting type {} index {}", (int) descriptorType, (int) descriptorIndex);

            if (descriptorType == 0x01) {
                // Device Descriptor
                responseData = &MouseDeviceDescriptor;
                responseDataSize = sizeof(MouseDeviceDescriptor);
                std::println("   - Responding with DEVICE Descriptor.");
            } else if (descriptorType == 0x02) {
                // Configuration Descriptor
                responseData = &MouseConfigurationDescriptor;
                responseDataSize = sizeof(MouseConfigurationDescriptor);
                std::println("   - Responding with CONFIGURATION Descriptor.");
            } else if (descriptorType == 0x03) {
                // String Descriptor
                switch (static_cast<USBStrings::Indices>(descriptorIndex)) {
                    case USBStrings::Indices::LangID:
                        responseData = &USBStrings::LangID;
                        responseDataSize = sizeof(USBStrings::LangID);
                        std::println("   - Responding with LANGID String Descriptor.");
                        break;
                    case USBStrings::Indices::Manufacturer:
                        responseData = &USBStrings::Manufacturer;
                        responseDataSize = sizeof(USBStrings::Manufacturer);
                        std::println("   - Responding with MANUFACTURER String Descriptor.");
                        break;
                    case USBStrings::Indices::Product:
                        responseData = &USBStrings::Product;
                        responseDataSize = sizeof(USBStrings::Product);
                        std::println("   - Responding with PRODUCT String Descriptor.");
                        break;
                    case USBStrings::Indices::SerialNumber:
                        responseData = &USBStrings::SerialNumber;
                        responseDataSize = sizeof(USBStrings::SerialNumber);
                        std::println("   - Responding with SERIALNUMBER String Descriptor.");
                        break;
                    case USBStrings::Indices::Interface:
                        responseData = &USBStrings::Interface;
                        responseDataSize = sizeof(USBStrings::Interface);
                        std::println("   - Responding with INTERFACE String Descriptor.");
                        break;
                }
            } else if (descriptorType == 0x22) {
                // HID Report Descriptor
                responseData = ReportDescriptor;
                responseDataSize = sizeof(ReportDescriptor);
                std::println("   - Responding with HID REPORT Descriptor bytes).");
            }
        }
        if (s.mRequestType == DeviceToInterface && s.mRequest == 0x06) {
            const auto descriptorType = (uint8_t) (s.mValue >> 8);
            //const uint8_t descriptorIndex = (uint8_t) (s.mValue & 0xFF);
            if (descriptorType == 0x22) { // HID report
                responseData = ReportDescriptor;
                responseDataSize = sizeof(ReportDescriptor);
                std::println("   - Responding with HID REPORT Descriptor bytes.");
            }
        }

        if (responseData) {
            const auto actualLength = std::min<uint32_t>(s.mLength, responseDataSize);
            USBIP::USBIP_RET_SUBMIT response{
                .mActualLength = to_network(actualLength), // 'use URB actual_length', intentionally not network byte order
                .mNumberOfPackets = 0xFFFF'FFFF, // Special value for non-ISO transfers
            };
            response.mHeader.mSequenceNumber = command.mHeader.mSequenceNumber;
            if (!send_all(sock, response)) return false;
            if (!send_all(sock, responseData, actualLength)) return false;
            return true;
        }

        USBIP::USBIP_RET_SUBMIT response{};
        response.mHeader.mSequenceNumber = command.mHeader.mSequenceNumber;

        // FIXME: just ack any zero-length messages, like idle/status
        if (command.mTransferBufferLength == 0) {
            if (!send_all(sock, response)) return false;
            return true;
        }
        // If it's a request we don't handle, fail gracefully
        std::println(stderr, "   - Error: Unhandled Control Request (0x{:x}/0x{:x}).",
                     (int) command.mSetup.mRequestType,
                     (int) command.mSetup.mRequest);
        response.mStatus = 0xFFFFFFFF;
        if (!send_all(sock, response)) return false;
        return true;
    }

    if (endpoint == 1 && command.mHeader.mDirection == USBIP::Direction::In) {
        const uint8_t mouse_report[] = {0x00, 0x01, 0x01};
        uint32_t report_len = sizeof(mouse_report);
        uint32_t final_len = std::min(report_len, transferBufferLength);
        USBIP::USBIP_RET_SUBMIT response{
            .mActualLength = to_network(final_len),
            .mNumberOfPackets = 0xFFFF'FFFF, // Special value for non-ISO transfers
        };
        response.mHeader.mSequenceNumber = command.mHeader.mSequenceNumber;
        if (!send_all(sock, response)) return false;
        if (!send_all(sock, mouse_report, final_len)) return false;
        return true;
    }

    // Unhandled endpoint or direction (e.g., EP1 OUT, which is not a standard mouse feature)
    std::println(stderr, "-> Error: Unhandled URB CMD (EP: {}, Dir: {}).", endpoint,
                 std::to_underlying(command.mHeader.mDirection));
    USBIP::USBIP_RET_SUBMIT response{};
    response.mHeader.mSequenceNumber = command.mHeader.mSequenceNumber;
    if (!send_all(sock, response)) return false;
    return true;
}


void handle_usbip_connection(SOCKET client_socket) {
    std::println("\n--- USB/IP Connection Established ---");

    while (true) {
        union {
            USBIP::CommandCode mCommandCode{};
            USBIP::OP_REQ_DEVLIST mOP_REQ_DEVLIST;
            USBIP::OP_REQ_IMPORT mOP_REQ_IMPORT;
            USBIP::USBIP_CMD_SUBMIT mUSBIP_CMD_SUBMIT;
            USBIP::USBIP_CMD_UNLINK mUSBIP_CMD_UNLINK;
        } request;

        if (!recv_all(client_socket, &request.mCommandCode)) {
            std::println(stderr, "Client disconnected or recv failed.");
            break; // Exit loop on failure/disconnect
        }

        const auto recv_remainder = []<class T>(SOCKET sock, T *what) {
            return recv_all(sock, reinterpret_cast<char *>(what) + sizeof(USBIP::CommandCode),
                            sizeof(T) - sizeof(USBIP::CommandCode));
        };

        switch (request.mCommandCode) {
            case USBIP::CommandCode::OP_REQ_DEVLIST:
                if (!recv_remainder(client_socket, &request.mOP_REQ_DEVLIST)) break;
                if (!handle_devlist_request(client_socket)) break;
                continue;
            case USBIP::CommandCode::OP_REQ_IMPORT:
                if (!recv_remainder(client_socket, &request.mOP_REQ_IMPORT)) break;
                if (!handle_import_request(client_socket, request.mOP_REQ_IMPORT.mBusID)) break;
                continue;
            case USBIP::CommandCode::USBIP_CMD_SUBMIT:
                if (!recv_remainder(client_socket, &request.mUSBIP_CMD_SUBMIT)) break;
                if (!handle_submit_request(client_socket, request.mUSBIP_CMD_SUBMIT)) break;
                continue;
            case USBIP::CommandCode::USBIP_CMD_UNLINK: {
                if (!recv_remainder(client_socket, &request.mUSBIP_CMD_UNLINK)) break;
                std::println("-> Received CMD_UNLINK. Sending RET_UNLINK.");
                USBIP::USBIP_RET_UNLINK response{};
                response.mHeader.mSequenceNumber = request.mUSBIP_CMD_UNLINK.mHeader.mSequenceNumber;
                if (!send_all(client_socket, response)) break;
                continue;
            }
            case USBIP::CommandCode::USBIP_RET_SUBMIT:
            case USBIP::CommandCode::USBIP_RET_UNLINK:
                std::println(stderr, "-> Received a RET instead of a CMD");
                break;
            default:
                std::println(stderr, "Unhandled USB/IP command code: 0x{:x}",
                             std::to_underlying(request.mCommandCode));
                break;
        }
        // We 'continue' from the successful cases, so if we reach here, we've failed.
        break;
    }

    std::println("--- USB/IP Connection Closed ---");
    closesocket(client_socket);
}

int main() {
    // Avoid need for explicit flush after each println
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::println(stderr, "WSAStartup failed.");
        return 1;
    }

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET) {
        std::println(stderr, "socket failed with error: {}", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_addr.sin_port = 0;

    if (bind(listen_socket, (sockaddr *) &server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::println(stderr, "bind failed with error: {}", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::println(stderr, "listen failed with error: {}", WSAGetLastError());
        closesocket(listen_socket);
        WSACleanup();
        return 1;
    }

    int server_addr_len = sizeof(server_addr);
    getsockname(listen_socket, (sockaddr *) &server_addr, &server_addr_len);

    std::println("USB/IP Server listening on port {}...", ntohs(server_addr.sin_port));

    SOCKET client_socket;
    while (true) {
        client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == INVALID_SOCKET) {
            std::println(stderr, "accept failed with error: {}", WSAGetLastError());
            continue;
        }
        handle_usbip_connection(client_socket);
    }

    closesocket(listen_socket);
    WSACleanup();
    return 0;
}
