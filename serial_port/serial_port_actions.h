#ifndef SERIAL_PORT_ACTIONS_H
#define SERIAL_PORT_ACTIONS_H

#include <QObject>
#include "serial_port_actions_direct.h"
#include "rep_serial_port_actions_replica.h"
#include "websocketiodevice.h"

class SerialPortActions : public QObject
{
    Q_OBJECT
public:
    explicit SerialPortActions(QString peerAddress="", QObject *parent=nullptr);
    ~SerialPortActions();

    bool get_serialPortAvailable();
    bool set_serialPortAvailable(bool value);
    bool get_setRequestToSend();
    bool set_setRequestToSend(bool value);
    bool get_setDataTerminalReady();
    bool set_setDataTerminalReady(bool value);

    bool get_add_iso14230_header();
    bool set_add_iso14230_header(bool value);
    bool get_is_iso14230_connection();
    bool set_is_iso14230_connection(bool value);
    bool get_is_can_connection();
    bool set_is_can_connection(bool value);
    bool get_is_iso15765_connection();
    bool set_is_iso15765_connection(bool value);
    bool get_is_29_bit_id();
    bool set_is_29_bit_id(bool value);

    bool get_use_openport2_adapter();
    bool set_use_openport2_adapter(bool value);

    int  get_requestToSendEnabled();
    bool set_requestToSendEnabled(int value);
    int  get_requestToSendDisabled();
    bool set_requestToSendDisabled(int value);
    int  get_dataTerminalEnabled();
    bool set_dataTerminalEnabled(int value);
    int  get_dataTerminalDisabled();
    bool set_dataTerminalDisabled(int value);

    uint8_t get_iso14230_startbyte();
    bool    set_iso14230_startbyte(uint8_t value);
    uint8_t get_iso14230_tester_id();
    bool    set_iso14230_tester_id(uint8_t value);
    uint8_t get_iso14230_target_id();
    bool    set_iso14230_target_id(uint8_t value);

    QByteArray get_ssm_receive_header_start();
    bool       set_ssm_receive_header_start(QByteArray value);

    QStringList get_serial_port_list();
    bool        set_serial_port_list(QStringList value);
    QString get_openedSerialPort();
    bool    set_openedSerialPort(QString value);
    QString get_subaru_02_16bit_bootloader_baudrate();
    bool    set_subaru_02_16bit_bootloader_baudrate(QString value);
    QString get_subaru_04_16bit_bootloader_baudrate();
    bool    set_subaru_04_16bit_bootloader_baudrate(QString value);
    QString get_subaru_02_32bit_bootloader_baudrate();
    bool    set_subaru_02_32bit_bootloader_baudrate(QString value);
    QString get_subaru_04_32bit_bootloader_baudrate();
    bool    set_subaru_04_32bit_bootloader_baudrate(QString value);
    QString get_subaru_05_32bit_bootloader_baudrate();
    bool    set_subaru_05_32bit_bootloader_baudrate(QString value);

    QString get_subaru_02_16bit_kernel_baudrate();
    bool    set_subaru_02_16bit_kernel_baudrate(QString value);
    QString get_subaru_04_16bit_kernel_baudrate();
    bool    set_subaru_04_16bit_kernel_baudrate(QString value);
    QString get_subaru_02_32bit_kernel_baudrate();
    bool    set_subaru_02_32bit_kernel_baudrate(QString value);
    QString get_subaru_04_32bit_kernel_baudrate();
    bool    set_subaru_04_32bit_kernel_baudrate(QString value);
    QString get_subaru_05_32bit_kernel_baudrate();
    bool    set_subaru_05_32bit_kernel_baudrate(QString value);

    QString get_can_speed();
    bool    set_can_speed(QString value);

    QString get_serial_port_baudrate();
    bool    set_serial_port_baudrate(QString value);
    QString get_serial_port_linux();
    bool    set_serial_port_linux(QString value);
    QString get_serial_port_windows();
    bool    set_serial_port_windows(QString value);
    QString get_serial_port();
    bool    set_serial_port(QString value);
    QString get_serial_port_prefix();
    bool    set_serial_port_prefix(QString value);
    QString get_serial_port_prefix_linux();
    bool    set_serial_port_prefix_linux(QString value);
    QString get_serial_port_prefix_win();
    bool    set_serial_port_prefix_win(QString value);

    uint32_t get_can_source_address();
    bool     set_can_source_address(uint32_t value);
    uint32_t get_can_destination_address();
    bool     set_can_destination_address(uint32_t value);
    uint32_t get_iso15765_source_address();
    bool     set_iso15765_source_address(uint32_t value);
    uint32_t get_iso15765_destination_address();
    bool     set_iso15765_destination_address(uint32_t value);

    bool is_serial_port_open(void);
    int change_port_speed(QString portSpeed);
    int fast_init(QByteArray output);
    int set_lec_lines(int lec1, int lec2);
    int pulse_lec_1_line(int timeout);
    int pulse_lec_2_line(int timeout);

    bool reset_connection(void);

    QByteArray read_serial_data(uint32_t datalen, unsigned long timeout);
    QByteArray write_serial_data(QByteArray output);
    QByteArray write_serial_data_echo_check(QByteArray output);

    int clear_rx_buffer(void);
    int clear_tx_buffer(void);

    int send_periodic_j2534_data(QByteArray output, int timeout);
    int stop_periodic_j2534_data(void);

    QStringList check_serial_ports(void);
    QString open_serial_port(void);

private:
    SerialPortActionsDirect        *serial_direct;
    SerialPortActionsRemoteReplica *serial_remote;
    QString peerAddress;

    bool isDirectConnection(void);

    const QString autodiscoveryMessage = "FastECU_PTP_Autodiscovery";
    const QString remoteObjectName = "FastECU";
    const int heartbeatInterval = 1000;
    QWebSocket *webSocket;
    WebSocketIoDevice *socket;
    QRemoteObjectNode node;
    void startRemote(void);
    void startOverNetwok(void);
    void startLocal(void);
    void sendAutoDiscoveryMessage();

};

#endif // SERIAL_PORT_ACTIONS_H