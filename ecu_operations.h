#ifndef ECU_OPERATIONS_H
#define ECU_OPERATIONS_H

#include <QMessageBox>
#include <QProgressBar>
#include <QTextEdit>
#include <QVBoxLayout>

#include <kernelcomms.h>
#include <kernelmemorymodels.h>
#include <file_actions.h>
#include <serial_port_actions.h>

class EcuOperations : public QWidget
{
    Q_OBJECT

public:
    explicit EcuOperations(QWidget *ui, SerialPortActions *serial, QString mcu_type_string, int mcu_type_index);
    ~EcuOperations();

    bool kill_process = false;

    QByteArray request_kernel_init();
    QByteArray request_kernel_id();

    int read_mem_16bit_kline(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length);
    int read_mem_32bit_kline(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length);
    int read_mem_32bit_can(FileActions::EcuCalDefStructure *ecuCalDef, uint32_t start_addr, uint32_t length);
    int write_mem_16bit_kline(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int write_mem_32bit_kline(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int write_mem_32bit_can(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write);
    int compare_mem_32bit_kline(QString mcu_type_string);
    int compare_mem_32bit_can(QString mcu_type_string);

private:
    void closeEvent(QCloseEvent *bar);

    int mcu_type_index;
    QString mcu_type_string;

    #define STATUS_SUCCESS							0x00
    #define STATUS_ERROR							0x01

    bool request_denso_kernel_init = false;
    bool request_denso_kernel_id = false;
    bool test_write = false;

    uint32_t flashbytescount = 0;
    uint32_t flashbytesindex = 0;

    uint16_t serial_read_extra_short_timeout = 50;
    uint16_t serial_read_short_timeout = 200;
    uint16_t serial_read_medium_timeout = 400;
    uint16_t serial_read_long_timeout = 800;
    uint16_t serial_read_extra_long_timeout = 3000;


    SerialPortActions *serial;
    FileActions *fileActions;

    QWidget *flash_window;
    //QProgressBar *progressbar;

    int get_changed_blocks_16bit_kline(const uint8_t *src, int *modified);
    int get_changed_blocks_32bit_kline(const uint8_t *src, int *modified);
    int get_changed_blocks_32bit_can(const uint8_t *src, int *modified);
    int check_romcrc_16bit_kline(const uint8_t *src, uint32_t start, uint32_t len, int *modified);
    int check_romcrc_32bit_kline(const uint8_t *src, uint32_t start, uint32_t len, int *modified);
    int check_romcrc_32bit_can(const uint8_t *src, uint32_t start, uint32_t len, int *modified);
    int npk_raw_flashblock_16bit_kline(const uint8_t *src, uint32_t start, uint32_t len);
    int npk_raw_flashblock_32bit_kline(const uint8_t *src, uint32_t start, uint32_t len);
    int npk_raw_flashblock_32bit_can(const uint8_t *src, uint32_t start, uint32_t len);
    int reflash_block_16bit_kline(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool practice);
    int reflash_block_32bit_kline(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool practice);
    int reflash_block_32bit_can(const uint8_t *newdata, const struct flashdev_t *fdt, unsigned blockno, bool practice);

    QString parse_message_to_hex(QByteArray received);
    void send_log_window_message(QString message, bool timestamp, bool linefeed);
    void set_progressbar_value(int value);

    uint8_t cks_add8(QByteArray chksum_data, unsigned len);
    void init_crc16_tab(void);
    uint16_t crc16(const uint8_t *data, uint32_t siz);
    uint8_t calculate_checksum(QByteArray output, bool dec_0x100);
    unsigned int crc32(const unsigned char *buf, unsigned int len);

signals:

private slots:
    void delay(int n);

};

#endif // ECU_OPERATIONS_H
