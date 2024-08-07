#include "ecu_operations_subaru.h"
#include <ui_ecu_operations.h>

EcuOperationsSubaru::EcuOperationsSubaru(SerialPortActions *serial, FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::EcuOperationsWindow)
{
    ui->setupUi(this);
    //this->setParent(parent);

    if (cmd_type == "test_write")
        this->setWindowTitle("Test write ROM " + ecuCalDef->FileName + " to ECU");
    else if (cmd_type == "write")
        this->setWindowTitle("Write ROM " + ecuCalDef->FileName + " to ECU");
    else
        this->setWindowTitle("Read ROM from ECU");
    this->show();
    this->serial = serial;

    int result = 0;

    ui->progressbar->setValue(0);
/*
    QFile file("./kernels/subarucan_encrypted.bin");
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
        //return STATUS_ERROR;
    }
    else
    {
        QByteArray kernel_data;
        uint32_t file_len = file.size();
        uint32_t pl_len = (file_len + 3) & ~3;
        QByteArray pl_encr = file.readAll();
        uint32_t len = pl_len &= ~3;
        unsigned char data[file_len];
        for (uint32_t i = 0; i < file_len; i++)
        {
            data[i] = (uint8_t)pl_encr.at(i);
        }
        kernel_data = subaru_denso_transform_wrx04_kernel(data, len, false);
        QFile dest_file("./kernels/subarucan_decrypted.bin");
        if (!dest_file.open(QIODevice::ReadWrite ))
        {
            send_log_window_message("Unable to open kernel file for reading", true, true);
            //return STATUS_ERROR;
        }
        else
        {
            dest_file.resize(0);
            dest_file.write(kernel_data);

            dest_file.close();
        }
    }
*/
    result = ecu_functions(ecuCalDef, cmd_type);

    if (result == STATUS_SUCCESS)
    {
        QMessageBox::information(this, tr("ECU Operation"), "ECU operation was succesful, press OK to exit");
        this->accept();
        //this->close();
    }
    else
    {
        QMessageBox::warning(this, tr("ECU Operation"), "ECU operation failed, press OK to exit and try again");
    }
}

EcuOperationsSubaru::~EcuOperationsSubaru()
{

}

void EcuOperationsSubaru::closeEvent(QCloseEvent *bar)
{
    kill_process = true;
    ecuOperations->kill_process = true;
}

void EcuOperationsSubaru::check_mcu_type(QString flash_method)
{
    if (flash_method.startsWith("wrx02"))
        mcu_type_string = "MC68HC16Y5";
    if (flash_method.startsWith("wrx04"))
        mcu_type_string = "MC68HC16Y5";
    if (flash_method.startsWith("fxt02"))
        mcu_type_string = "SH7055";
    if (flash_method.startsWith("sti04"))
        mcu_type_string = "SH7055";
    if (flash_method.startsWith("sti05"))
        mcu_type_string = "SH7058";
    if (flash_method.startsWith("subarucan"))
        mcu_type_string = "SH7058";

    if (flash_method == "uj20")
        mcu_type_string = "M32R_UJ20";
    if (flash_method == "uj30")
        mcu_type_string = "M32R_UJ30";
    if (flash_method == "uj40")
        mcu_type_string = "M32R_UJ40";
    if (flash_method == "uj70")
        mcu_type_string = "M32R_UJ70";

    if (flash_method == "mitsubootloader")
        mcu_type_string = "M32R_UJ70";

    if (flash_method == "hitachi")
        mcu_type_string = "M32R_UJ70";
    if (flash_method == "hitachican")
        mcu_type_string = "M32R_UJ70";

    if (flash_method == "sh7055_denso_can_recovery")
        mcu_type_string = "SH7055";
    if (flash_method == "sh7058_denso_can_recovery")
        mcu_type_string = "SH7058";
    if (flash_method == "sh7058s_denso_can_recovery")
        mcu_type_string = "SH7058";

    mcu_type_index = 0;

    while (flashdevices[mcu_type_index].name != 0)
    {
        //qDebug() << flashdevices[mcu_type_index].name;
        if (flashdevices[mcu_type_index].name == mcu_type_string)
            break;
        mcu_type_index++;
    }
}

int EcuOperationsSubaru::ecu_functions(FileActions::EcuCalDefStructure *ecuCalDef, QString cmd_type)
{
    //QByteArray romdata;
    int test_write = 0;
    int result = STATUS_ERROR;

    flash_method = ecuCalDef->RomInfo.at(FlashMethod);
    kernel = ecuCalDef->Kernel;
    check_mcu_type(flash_method);

    if (cmd_type == "read")
    {
        send_log_window_message("Read memory with flashmethod " + flash_method + " and kernel " + ecuCalDef->Kernel, true, true);
        qDebug() << "Read memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;

        QMessageBox::information(this, tr("Read ROM"), "Press OK to start read countdown");
    }
    else if (cmd_type == "test_write")
    {
        //romdata = ecuCalDef->FullRomData;
        test_write = 1;
        send_log_window_message("Test write memory with flashmethod " + flash_method + " and kernel " + ecuCalDef->Kernel, true, true);
        qDebug() << "Test write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;

        QMessageBox::information(this, tr("Test write ROM"), "Press OK to start test write countdown");
    }
    else if (cmd_type == "write")
    {
        //romdata = ecuCalDef->FullRomData;
        test_write = 0;
        send_log_window_message("Write memory with flashmethod " + flash_method + " and kernel " + ecuCalDef->Kernel, true, true);
        qDebug() << "Write memory with flashmethod" << flash_method << "and kernel" << ecuCalDef->Kernel;

        QMessageBox::information(this, tr("Write ROM"), "Press OK to start write countdown!");
    }

    ecuOperations = new EcuOperations(this, serial, mcu_type_string, mcu_type_index);
    kernel_alive = false;

    if (flash_method.startsWith("wrx02"))
    {
        serial->open_serial_port();

        send_log_window_message("Connecting to Subaru 01 16-bit K-Line bootloader, please wait...", true, true);
        result = connect_bootloader_subaru_denso_kline_02_16bit();
        if (result == STATUS_SUCCESS && !kernel_alive)
        {
            send_log_window_message("Initializing Subaru 01 16-bit K-Line kernel upload, please wait...", true, true);
            result = upload_kernel_subaru_denso_kline_02_16bit(kernel);
        }
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
            {
                send_log_window_message("Reading ROM from Subaru 01 16-bit using K-Line", true, true);
                result = read_rom_subaru_denso_kline_02_16bit(ecuCalDef);
            }
            else if (cmd_type == "test_write" || cmd_type == "write")
            {
                send_log_window_message("Writing ROM to Subaru 01 16-bit using K-Line", true, true);
                result = write_rom_subaru_denso_kline_02_16bit(ecuCalDef, test_write);
            }
        }
        return result;
    }
    else if (flash_method.startsWith("wrx04"))
    {
        serial->open_serial_port();

        send_log_window_message("Connecting to Subaru 04 16-bit K-Line bootloader, please wait...", true, true);
        result = connect_bootloader_subaru_denso_kline_04_16bit();
        if (result == STATUS_SUCCESS && !kernel_alive)
        {
            send_log_window_message("Initializing Subaru 04 16-bit K-Line kernel upload, please wait...", true, true);
            result = upload_kernel_subaru_denso_kline_04_16bit(kernel);
        }
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
            {
                send_log_window_message("Reading ROM from Subaru 04 16-bit using K-Line", true, true);
                result = read_rom_subaru_denso_kline_04_16bit(ecuCalDef);
            }
            else if (cmd_type == "test_write" || cmd_type == "write")
            {
                send_log_window_message("Writing ROM to Subaru 04 16-bit using K-Line", true, true);
                result = write_rom_subaru_denso_kline_04_16bit(ecuCalDef, test_write);
            }
        }
        return result;
    }
    else if (flash_method.startsWith("fxt02"))
    {
        if (serial->is_can_connection)
        {
            serial->is_can_connection = true;
            serial->is_iso15765_connection = false;
            serial->is_29_bit_id = true;
            serial->can_speed = "500000";
            serial->can_source_address = 0xFFFFE;
            serial->can_destination_address = 0x21;
            serial->open_serial_port();

            send_log_window_message("Connecting to Subaru 02 32-bit CAN bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_denso_can_32bit();
        }
        else
        {
            serial->is_can_connection = false;
            serial->is_iso15765_connection = false;
            serial->is_iso14230_connection = true;
            serial->open_serial_port();

            send_log_window_message("Connecting to Subaru 02 32-bit K-Line bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_denso_kline_02_32bit();
        }
        if (result == STATUS_SUCCESS && !kernel_alive)
        {
            if (serial->is_can_connection)
            {
                send_log_window_message("Initializing Subaru 02 32-bit CAN kernel upload, please wait...", true, true);
                result = upload_kernel_subaru_denso_can_32bit(kernel);
            }
            else
            {
                send_log_window_message("Initializing Subaru 02 32-bit K-Line kernel upload, please wait...", true, true);
                result = upload_kernel_subaru_denso_kline_02_32bit(kernel);
            }
        }
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
            {
                if (serial->is_can_connection)
                {
                    send_log_window_message("Reading ROM from Subaru 02 32-bit using CAN", true, true);
                    result = read_rom_subaru_denso_can_32bit(ecuCalDef);
                }
                else
                {
                    send_log_window_message("Reading ROM from Subaru 02 32-bit using K-Line", true, true);
                    result = read_rom_subaru_denso_kline_32bit(ecuCalDef);
                }
            }
            else if (cmd_type == "test_write" || cmd_type == "write")
            {
                if (serial->is_can_connection)
                {
                    send_log_window_message("Writing ROM to Subaru 02 32-bit using CAN", true, true);
                    result = write_rom_subaru_denso_can_32bit(ecuCalDef, test_write);
                }
                else
                {
                    send_log_window_message("Writing ROM to Subaru 02 32-bit using K-Line", true, true);
                    result = write_rom_subaru_denso_kline_32bit(ecuCalDef, test_write);
                }
            }
        }
        return result;
    }
    else if (flash_method.startsWith("sti04"))
    {
        if (serial->is_can_connection)
        {
            serial->is_can_connection = true;
            serial->is_iso15765_connection = false;
            serial->is_29_bit_id = true;
            serial->can_speed = "500000";
            serial->can_source_address = 0xFFFFE;
            serial->can_destination_address = 0x21;
            serial->open_serial_port();

            send_log_window_message("Connecting to Subaru 04 32-bit CAN bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_denso_can_32bit();
        }
        else
        {
            serial->is_can_connection = false;
            serial->is_iso15765_connection = false;
            serial->is_iso14230_connection = true;
            serial->open_serial_port();

            send_log_window_message("Connecting to Subaru 04 32-bit K-line bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_denso_kline_04_32bit();
        }
        if (result == STATUS_SUCCESS && !kernel_alive)
        {
            if (serial->is_can_connection)
            {
                send_log_window_message("Initializing Subaru 04 32-bit CAN kernel upload, please wait...", true, true);
                result = upload_kernel_subaru_denso_can_32bit(kernel);
            }
            else
            {
                send_log_window_message("Initializing Subaru 04 32-bit K-Line kernel upload, please wait...", true, true);
                result = upload_kernel_subaru_denso_kline_04_32bit(kernel);
            }
        }
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
            {
                if (serial->is_can_connection)
                {
                    send_log_window_message("Reading ROM from Subaru 04 32-bit using CAN", true, true);
                    result = read_rom_subaru_denso_can_32bit(ecuCalDef);
                }
                else
                {
                    send_log_window_message("Reading ROM from Subaru 04 32-bit using K-Line", true, true);
                    result = read_rom_subaru_denso_kline_32bit(ecuCalDef);
                }
            }
            else if (cmd_type == "test_write" || cmd_type == "write")
            {
                if (serial->is_can_connection)
                {
                    send_log_window_message("Writing ROM to Subaru 04 32-bit using CAN", true, true);
                    result = write_rom_subaru_denso_can_32bit(ecuCalDef, test_write);
                }
                else
                {
                    send_log_window_message("Writing ROM to Subaru 04 32-bit using K-Line", true, true);
                    result = write_rom_subaru_denso_kline_32bit(ecuCalDef, test_write);
                }
            }
        }
        return result;
    }
    else if (flash_method.startsWith("sti05"))
    {
        if (serial->is_can_connection)
        {
            serial->is_can_connection = true;
            serial->is_iso15765_connection = false;
            serial->is_29_bit_id = true;
            serial->can_speed = "500000";
            serial->can_source_address = 0xFFFFE;
            serial->can_destination_address = 0x21;
            serial->open_serial_port();

            send_log_window_message("Connecting to Subaru 05 32-bit CAN bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_denso_can_32bit();
        }
        else
        {
            serial->is_can_connection = false;
            serial->is_iso15765_connection = false;
            serial->is_iso14230_connection = true;
            serial->open_serial_port();

            send_log_window_message("Connecting to Subaru 05 32-bit K-line bootloader, please wait...", true, true);
            result = connect_bootloader_subaru_denso_kline_04_32bit();
        }
        if (result == STATUS_SUCCESS && !kernel_alive)
        {
            if (serial->is_can_connection)
            {
                send_log_window_message("Initializing Subaru 05 32-bit CAN kernel upload, please wait...", true, true);
                result = upload_kernel_subaru_denso_can_32bit(kernel);
            }
            else
            {
                send_log_window_message("Initializing Subaru 05 32-bit K-Line kernel upload, please wait...", true, true);
                result = upload_kernel_subaru_denso_kline_04_32bit(kernel);
            }
        }
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
            {
                if (serial->is_can_connection)
                {
                    send_log_window_message("Reading ROM from Subaru 05 32-bit using CAN", true, true);
                    result = read_rom_subaru_denso_can_32bit(ecuCalDef);
                }
                else
                {
                    send_log_window_message("Reading ROM from Subaru 05 32-bit using K-Line", true, true);
                    result = read_rom_subaru_denso_kline_32bit(ecuCalDef);
                }
            }
            else if (cmd_type == "test_write" || cmd_type == "write")
            {
                if (serial->is_can_connection)
                {
                    send_log_window_message("Writing ROM to Subaru 05 32-bit using CAN", true, true);
                    result = write_rom_subaru_denso_can_32bit(ecuCalDef, test_write);
                }
                else
                {
                    send_log_window_message("Writing ROM to Subaru 05 32-bit using K-Line", true, true);
                    result = write_rom_subaru_denso_kline_32bit(ecuCalDef, test_write);
                }
            }
        }
        return result;
    }
    else if (flash_method.startsWith("subarucan"))
    {
        send_log_window_message("Connecting to Subaru 32-bit CAN bootloader, please wait...", true, true);

        bool denso_can_bootloader = false;
        serial->is_29_bit_id = false;
        serial->iso15765_source_address = 0x7E0;
        serial->iso15765_destination_address = 0x7E8;
        serial->can_speed = "500000";
        if (serial->is_can_connection)
        {
            denso_can_bootloader = true;
            serial->is_29_bit_id = true;
            serial->is_can_connection = true;
            serial->is_iso15765_connection = false;
            //serial->iso15765_source_address = 0xFFFFFFFF;
            //serial->iso15765_destination_address = 0x00000021;
            serial->can_source_address = 0xFFFFFFFF;
            serial->can_destination_address = 0x00000021;
        }
        serial->open_serial_port();

        if (denso_can_bootloader)
            result = connect_bootloader_subaru_denso_can_32bit();
        else
            result = connect_bootloader_subaru_denso_iso15765_32bit();
        if (result == STATUS_SUCCESS && !kernel_alive)
        {
            send_log_window_message("Initializing Subaru 32-bit CAN kernel upload, please wait...", true, true);
            if (denso_can_bootloader)
                result = upload_kernel_subaru_denso_can_32bit(kernel);
            else
                result = upload_kernel_subaru_denso_iso15765_32bit(kernel);
        }
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
            {
                send_log_window_message("Reading ROM from Subaru 32-bit using CAN", true, true);
                if (denso_can_bootloader)
                    result = read_rom_subaru_denso_can_32bit(ecuCalDef);
                else
                    result = read_rom_subaru_denso_iso15765_32bit(ecuCalDef);
            }
            else if (cmd_type == "test_write" || cmd_type == "write")
            {
                send_log_window_message("Writing ROM to Subaru 32-bit using CAN", true, true);
                if (denso_can_bootloader)
                    result = write_rom_subaru_denso_can_32bit(ecuCalDef, test_write);
                else
                    result = write_rom_subaru_denso_iso15765_32bit(ecuCalDef, test_write);
            }
        }
        return result;
    }
    else if (flash_method == "uj20")
    {
        serial->is_iso14230_connection = true;
        serial->open_serial_port();

        if (cmd_type == "read")
        {
            send_log_window_message("Initializing Subaru 1999-2000 Hitachi UJ WA12212920/128KB read mode using K-Line, please wait...", true, true);
            result = initialize_read_mode_subaru_uj20_30_40_70_kline();
            if (result == STATUS_SUCCESS)
            {
                send_log_window_message("Reading ROM from Subaru 1999-2000 Hitachi UJ WA12212920/128KB using K-Line", true, true);
                result = read_rom_subaru_uj20_30_40_70_kline(ecuCalDef);
            }
            result = uninitialize_read_mode_subaru_uj20_30_40_70_kline();
        }
        else if (cmd_type == "write")
        {
            send_log_window_message("Initializing Subaru 1999-2000 Hitachi UJ WA12212920/128KB flash mode using K-Line, please wait...", true, true);
            result = initialize_flash_mode_subaru_uj20_30_40_70_kline();
            if (result == STATUS_SUCCESS)
            {

            }
        }
        return result;
    }
    else if (flash_method == "uj30")
    {
        serial->is_iso14230_connection = true;
        serial->open_serial_port();

        if (cmd_type == "read")
        {
            send_log_window_message("Initializing Subaru 2000-2002 Hitachi UJ WA12212930/256KB read mode using K-Line, please wait...", true, true);
            result = initialize_read_mode_subaru_uj20_30_40_70_kline();
            if (result == STATUS_SUCCESS)
            {
                send_log_window_message("Reading ROM from Subaru 2000-2002 Hitachi UJ WA12212930/256KB using K-Line", true, true);
                result = read_rom_subaru_uj20_30_40_70_kline(ecuCalDef);
            }
            result = uninitialize_read_mode_subaru_uj20_30_40_70_kline();
        }
        else if (cmd_type == "write")
        {
            send_log_window_message("Initializing Subaru 2000-2002 Hitachi UJ WA12212930/256KB flash mode using K-Line, please wait...", true, true);
            result = initialize_flash_mode_subaru_uj20_30_40_70_kline();
            if (result == STATUS_SUCCESS)
            {
                send_log_window_message("Writing ROM to Subaru 2000-2002 Hitachi UJ WA12212930/256KB using K-Line", true, true);
                result = write_rom_subaru_uj20_30_40_70_kline(ecuCalDef);
            }
        }
        return result;
    }
    else if (flash_method == "uj40")
    {
        serial->is_iso14230_connection = true;
        serial->open_serial_port();

        if (cmd_type == "read")
        {
            send_log_window_message("Initializing Subaru 2002-2005 Hitachi UJ WA12212940/384KB read mode using K-Line, please wait...", true, true);
            result = initialize_read_mode_subaru_uj20_30_40_70_kline();
            if (result == STATUS_SUCCESS)
            {
                send_log_window_message("Reading ROM from Subaru 2002-2005 Hitachi UJ WA12212940/384KB using K-Line", true, true);
                result = read_rom_subaru_uj20_30_40_70_kline(ecuCalDef);
            }
            result = uninitialize_read_mode_subaru_uj20_30_40_70_kline();
        }
        else if (cmd_type == "write")
        {
            send_log_window_message("Initializing Subaru 2002-2005 Hitachi UJ WA12212940/384KB flash mode using K-Line, please wait...", true, true);
            result = initialize_flash_mode_subaru_uj20_30_40_70_kline();
            if (result == STATUS_SUCCESS)
            {
                send_log_window_message("Writing ROM to Subaru 2002-2005 Hitachi UJ WA12212940/384KB using K-Line", true, true);
                result = write_rom_subaru_uj20_30_40_70_kline(ecuCalDef);
            }
        }
        return result;
    }
    else if (flash_method == "uj70")
    {
        serial->is_iso14230_connection = true;
        serial->open_serial_port();

        if (cmd_type == "read")
        {
            send_log_window_message("Initializing Subaru 2002-2005 Hitachi UJ WA12212970/512KB read mode using K-Line, please wait...", true, true);
            result = initialize_read_mode_subaru_uj20_30_40_70_kline();
            if (result == STATUS_SUCCESS)
            {
                send_log_window_message("Reading ROM from Subaru 2002-2005 Hitachi UJ WA12212970/512KB using K-Line", true, true);
                result = read_rom_subaru_uj20_30_40_70_kline(ecuCalDef);
            }
            result = uninitialize_read_mode_subaru_uj20_30_40_70_kline();
        }
        else if (cmd_type == "write")
        {
            send_log_window_message("Initializing Subaru 2002-2005 Hitachi UJ WA12212970/512KB flash mode using K-Line, please wait...", true, true);
            result = initialize_flash_mode_subaru_uj20_30_40_70_kline();
            if (result == STATUS_SUCCESS)
            {
                send_log_window_message("Writing ROM to Subaru 2002-2005 Hitachi UJ WA12212970/512KB using K-Line", true, true);
                result = write_rom_subaru_uj20_30_40_70_kline(ecuCalDef);
            }
        }
        return result;
    }
    else if (flash_method == "mitsubootloader")
    {
        serial->is_iso14230_connection = true;
        serial->open_serial_port();

        if (cmd_type == "read")
        {
            send_log_window_message("Initializing Subaru 2002-2005 Hitachi UJ WA12212970/512KB read mode using K-Line, please wait...", true, true);
            result = initialize_read_mode_subaru_uj20_30_40_70_kline();
            if (result == STATUS_SUCCESS)
            {
                send_log_window_message("Reading ROM from Subaru 2002-2005 Hitachi UJ WA12212970/512KB using K-Line", true, true);
                result = read_rom_subaru_uj20_30_40_70_kline(ecuCalDef);
            }
            result = uninitialize_read_mode_subaru_uj20_30_40_70_kline();
        }
        else if (cmd_type == "write")
        {
            send_log_window_message("Initializing Subaru 2002-2005 Hitachi UJ WA12212970/512KB flash mode using K-Line, please wait...", true, true);
            result = initialize_flash_mode_subaru_uj70_kline();
            if (result == STATUS_SUCCESS)
            {
                send_log_window_message("Writing ROM to Subaru 2002-2005 Hitachi UJ WA12212970/512KB using K-Line", true, true);
                result = write_rom_subaru_uj70_kline(ecuCalDef);
            }
        }
        return result;
    }
    else if (flash_method.endsWith("denso_can_recovery"))
    {
        send_log_window_message("Connecting to Subaru Denso 32-bit CAN bootloader, please wait...", true, true);

        serial->is_iso14230_connection = false;
        serial->is_can_connection = true;
        serial->is_iso15765_connection = false;
        serial->is_29_bit_id = true;
        serial->can_speed = "500000";
        serial->can_source_address = 0xFFFFE;
        serial->can_destination_address = 0x21;
        serial->open_serial_port();

        result = connect_bootloader_subaru_denso_can_recovery_32bit();

        if (result == STATUS_SUCCESS && !kernel_alive)
        {
            send_log_window_message("Initializing Subaru Denso 32-bit CAN kernel upload, please wait...", true, true);
            result = upload_kernel_subaru_denso_can_32bit(kernel);
        }
        if (result == STATUS_SUCCESS)
        {
            if (cmd_type == "read")
            {
                send_log_window_message("Reading ROM from Subaru 32-bit using Denso CAN", true, true);
                result = read_rom_subaru_denso_can_32bit(ecuCalDef);
            }
            else if (cmd_type == "test_write" || cmd_type == "write")
            {
                send_log_window_message("Writing ROM to Subaru 32-bit using Denso CAN", true, true);
                result = write_rom_subaru_denso_can_32bit(ecuCalDef, test_write);
            }
        }
        return result;
    }
    else
    {
        send_log_window_message("Unknown flashmethod " + flash_method, true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_denso_kline_02_16bit()
{
    QByteArray output;
    QByteArray received;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    // Change serial speed and set 'line end checks' to low level
    serial->change_port_speed("9600");
    serial->set_lec_lines(serial->requestToSendDisabled, serial->dataTerminalDisabled);

    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    delay(800);
    serial->pulse_lec_2_line(200);
    output.clear();
    for (uint8_t i = 0; i < subaru_16bit_bootloader_init.length(); i++)
    {
        output.append(subaru_16bit_bootloader_init[i]);
    }
    //received = serial->write_serial_data_echo_check(output);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent to bootloader: " + parse_message_to_hex(output), true, true);
    received = serial->read_serial_data(output.length(), serial_read_short_timeout);
    send_log_window_message("Response from bootloader: " + parse_message_to_hex(received), true, true);

    if (flash_method.endsWith("_ecutek"))
        subaru_16bit_bootloader_init_ok = subaru_16bit_bootloader_ecutek_init_ok;
    else
        subaru_16bit_bootloader_init_ok = subaru_16bit_bootloader_stock_init_ok;

    if (received.length() != 3 || !check_received_message(subaru_16bit_bootloader_init_ok, received))
    {
        send_log_window_message("Bad response from bootloader: " + parse_message_to_hex(received), true, true);
    }
    else
    {
        send_log_window_message("Connected to bootloader", true, true);
        return STATUS_SUCCESS;
    }

    delay(10);

    send_log_window_message("Checking if Kernel already uploaded, requesting kernel ID", true, true);
    serial->change_port_speed("39473");

    received = ecuOperations->request_kernel_id();
    qDebug() << parse_message_to_hex(received);
    if (received.length() > 0)
    {
        send_log_window_message("Kernel already uploaded, requesting kernel ID", true, true);
        delay(100);

        kernel_alive = true;
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_denso_kline_04_16bit()
{
    QByteArray output;
    QByteArray received;

    send_log_window_message("Connecting to Subaru 04 16-bit k-line bootloader, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR! Serial port is not open!", true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_denso_kline_02_32bit()
{
    QByteArray output;
    QByteArray received;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    // Change serial speed and set 'line end checks' to low level
    serial->change_port_speed("9600");
    serial->set_lec_lines(serial->requestToSendDisabled, serial->dataTerminalDisabled);

    //delay(10);

    // Start countdown
    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    // Connect to bootloader
    delay(800);

    serial->pulse_lec_2_line(200);
    output.clear();
    //for (uint8_t i = 0; i < (sizeof(subaru_16bit_bootloader_init) / sizeof(subaru_16bit_bootloader_init[0])); i++)
    for (uint8_t i = 0; i < subaru_16bit_bootloader_init.length(); i++)
    {
        output.append(subaru_16bit_bootloader_init[i]);
    }
    //received = serial->write_serial_data_echo_check(output);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent to bootloader: " + parse_message_to_hex(output), true, true);
    //delay(200);
    received = serial->read_serial_data(output.length(), serial_read_short_timeout);
    send_log_window_message("Response from bootloader: " + parse_message_to_hex(received), true, true);

    if (flash_method.endsWith("_ecutek"))
        subaru_16bit_bootloader_init_ok = subaru_16bit_bootloader_ecutek_init_ok;
    else
        subaru_16bit_bootloader_init_ok = subaru_16bit_bootloader_stock_init_ok;

    if (received.length() != 3 || !check_received_message(subaru_16bit_bootloader_init_ok, received))
    {
        send_log_window_message("Bad response from bootloader", true, true);
    }
    else
    {
        send_log_window_message("Connected to bootloader", true, true);
        return STATUS_SUCCESS;
    }

    send_log_window_message("Cannot connect to bootloader, testing if kernel is alive", true, true);

    serial->change_port_speed("62500");
    serial->add_iso14230_header = true;
    serial->iso14230_startbyte = 0x80;
    serial->iso14230_tester_id = 0xFC;
    serial->iso14230_target_id = 0x10;

    //delay(100);

    received = ecuOperations->request_kernel_init();
    if (received.length() > 0)
    {
        if ((uint8_t)received.at(1) == SID_KERNEL_INIT + 0x40)
        {
            send_log_window_message("Kernel already uploaded, requesting kernel ID", true, true);
            delay(100);

            received = ecuOperations->request_kernel_id();
            if (received == "")
                return STATUS_ERROR;

            received.remove(0, 2);
            send_log_window_message("Request kernel id ok: " + received, true, true);
            kernel_alive = true;
            return STATUS_SUCCESS;
        }
    }

    serial->add_iso14230_header = false;

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_denso_kline_04_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray seed;
    QByteArray seed_key;

    QString msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    serial->change_port_speed("62500");
    serial->add_iso14230_header = true;
    serial->iso14230_startbyte = 0x80;
    serial->iso14230_tester_id = 0xFC;
    serial->iso14230_target_id = 0x10;

    delay(100);

    received = ecuOperations->request_kernel_init();
    if (received.length() > 0)
    {
        if ((uint8_t)received.at(1) == SID_KERNEL_INIT + 0x40)
        {
            send_log_window_message("Kernel already uploaded, requesting kernel ID", true, true);
            delay(100);
            delay(100);

            received = ecuOperations->request_kernel_id();
            if (received == "")
                return STATUS_ERROR;

            received.remove(0, 2);
            send_log_window_message("Request kernel id ok: " + received, true, true);
            kernel_alive = true;
            return STATUS_SUCCESS;
        }
    }

    serial->change_port_speed("4800");
    serial->add_iso14230_header = false;
    delay(100);

    // SSM init
    received = send_subaru_sid_bf_ssm_init();
    //send_log_window_message("SID BF = " + parse_message_to_hex(received), true, true);
    if (received == "")
        return STATUS_ERROR;
    //qDebug() << "SID_BF received:" << parse_message_to_hex(received);
    received.remove(0, 8);
    received.remove(5, received.length() - 5);
    //qDebug() << "Received length:" << received.length();
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString ecuid = msg;
    //QString ecuid = QString("%1%2%3%4%5").arg(received.at(0),2,16,QLatin1Char('0')).toUpper().arg(received.at(1),2,16,QLatin1Char('0')).arg(received.at(2),2,16,QLatin1Char('0')).arg(received.at(3),2,16,QLatin1Char('0')).arg(received.at(4),2,16,QLatin1Char('0'));

    send_log_window_message("ECU ID = " + ecuid, true, true);

    // Start communication
    received = send_subaru_denso_sid_81_start_communication();
    //send_log_window_message("SID_81 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xC1)
        return STATUS_ERROR;

    send_log_window_message("Start communication ok", true, true);

    // Request timing parameters
    received = send_subaru_denso_sid_83_request_timings();
    //send_log_window_message("SID_83 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xC3)
        return STATUS_ERROR;

    send_log_window_message("Request timing parameters ok", true, true);

    // Request seed
    received = send_subaru_denso_sid_27_request_seed();
    //send_log_window_message("SID_27_01 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
        return STATUS_ERROR;

    send_log_window_message("Seed request ok", true, true);

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    if (flash_method.endsWith("_ecutek"))
        seed_key = subaru_denso_generate_ecutek_kline_seed_key(seed);
    else
        seed_key = subaru_denso_generate_kline_seed_key(seed);
    msg = parse_message_to_hex(seed_key);
    //qDebug() << "Calculated seed key: " + msg + ", sending to ECU";
    //send_log_window_message("Calculated seed key: " + msg + ", sending to ECU", true, true);
    send_log_window_message("Sending seed key", true, true);

    received = send_subaru_denso_sid_27_send_seed_key(seed_key);
    //send_log_window_message("SID_27_02 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
        return STATUS_ERROR;

    send_log_window_message("Seed key ok", true, true);

    // Start diagnostic session
    received = send_subaru_denso_sid_10_start_diagnostic();
    if (received == "" || (uint8_t)received.at(4) != 0x50)
        return STATUS_ERROR;

    //send_log_window_message("Starting diagnostic session", true, true);

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::connect_bootloader_subaru_denso_can_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    // Check if kernel already alive
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)(SID_START_COMM_CAN & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    qDebug() << "0x7A 0x00 response:" << parse_message_to_hex(received);
    send_log_window_message("0x7A 0x00 response: " + parse_message_to_hex(received), true, true);
    if ((uint8_t)received.at(0) == 0x7F && (uint8_t)received.at(2) == 0x34)
    {
        send_log_window_message("Kernel already uploaded", true, true);

        kernel_alive = true;
        return STATUS_SUCCESS;
    }

    output[4] = (uint8_t)((SID_ENTER_BL_CAN >> 8) & 0xFF);
    output[5] = (uint8_t)(SID_ENTER_BL_CAN & 0xFF);
    output[6] = (uint8_t)0x00;
    output[7] = (uint8_t)0x00;
    output[8] = (uint8_t)0x00;
    output[9] = (uint8_t)0x00;
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    send_log_window_message("0xFF 0x86 response: " + parse_message_to_hex(received), true, true);
    qDebug() << "0xFF 0x86 response:" << parse_message_to_hex(received);

    output[4] = (uint8_t)SID_START_COMM_CAN;
    output[5] = (uint8_t)(SID_CHECK_COMM_BL_CAN & 0xFF);
    output[6] = (uint8_t)0x00;
    output[7] = (uint8_t)0x00;
    output[8] = (uint8_t)0x00;
    output[9] = (uint8_t)0x00;
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    send_log_window_message("0x7A 0x90 response: " + parse_message_to_hex(received), true, true);
    qDebug() << "0x7A 0x90 response:" << parse_message_to_hex(received);
    if ((uint8_t)(received.at(1) & 0xF8) == 0x90)
    {
        send_log_window_message("Connected to bootloader, start kernel upload", true, true);
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_denso_can_iso15765_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    // Check if kernel already alive
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)(SID_START_COMM_CAN & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    qDebug() << "0x7A 0x00 response:" << parse_message_to_hex(received);
    if ((uint8_t)received.at(0) == 0x7F && (uint8_t)received.at(2) == 0x34)
    {
        send_log_window_message("Kernel already uploaded", true, true);

        kernel_alive = true;
        return STATUS_SUCCESS;
    }

    output[4] = (uint8_t)((SID_ENTER_BL_CAN >> 8) & 0xFF);
    output[5] = (uint8_t)(SID_ENTER_BL_CAN & 0xFF);
    output[6] = (uint8_t)0x00;
    output[7] = (uint8_t)0x00;
    output[8] = (uint8_t)0x00;
    output[9] = (uint8_t)0x00;
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    qDebug() << "0xFF 0x86 response:" << parse_message_to_hex(received);

    output[4] = (uint8_t)SID_START_COMM_CAN;
    output[5] = (uint8_t)(SID_CHECK_COMM_BL_CAN & 0xFF);
    output[6] = (uint8_t)0x00;
    output[7] = (uint8_t)0x00;
    output[8] = (uint8_t)0x00;
    output[9] = (uint8_t)0x00;
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    qDebug() << "0x7A 0x90 response:" << parse_message_to_hex(received);
    if ((uint8_t)(received.at(1) & 0xF8) == 0x90)
    {
        send_log_window_message("Connected to bootloader, start kernel upload", true, true);
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_denso_iso15765_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x07);
    output.append((uint8_t)0xE0);
    output.append((uint8_t)0x01);

    bool connected = false;
    int try_count = 0;
    int try_timeout = 250;
    // Start initialization

    serial->write_serial_data_echo_check(output);
    send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
    qDebug() << "Sent:" << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    //send_log_window_message(QString::number(try_count) + ": 0x01 response: " + parse_message_to_hex(received), true, true);
    //qDebug() << try_count << ": 0x01 response:" << parse_message_to_hex(received);
    while (received != "")
    {
        send_log_window_message(QString::number(try_count) + ": 0x01 response: " + parse_message_to_hex(received), true, true);
        qDebug() << try_count << ": 0x01 response:" << parse_message_to_hex(received);
        received = serial->read_serial_data(20, 10);
    }

    connected = false;
    try_count = 0;
    output.append((uint8_t)0x00);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(20, 10);
        //send_log_window_message(QString::number(try_count) + ": 0x01 0x00 response: " + parse_message_to_hex(received), true, true);
        //qDebug() << try_count << ": 0x01 0x00 response:" << parse_message_to_hex(received);
        while (received != "")
        {
            if (received != "")
                connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x01 0x00 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x01 0x00 response:" << parse_message_to_hex(received);
            received = serial->read_serial_data(20, 10);
        }
        try_count++;
        delay(try_timeout);
    }
    connected = false;
    try_count = 0;
    output[4] = ((uint8_t)0x10);
    output[5] = ((uint8_t)0x03);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(20, 10);
        //send_log_window_message(QString::number(try_count) + ": 0x10 0x03 response: " + parse_message_to_hex(received), true, true);
        //qDebug() << try_count << ": 0x10 0x03 response:" << parse_message_to_hex(received);
        while (received != "")
        {
            if (received != "")
                connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x10 0x03 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x10 0x03 response:" << parse_message_to_hex(received);
            received = serial->read_serial_data(20, 10);
        }
        try_count++;
        delay(try_timeout);
    }
    connected = false;
    try_count = 0;
    output[5] = ((uint8_t)0x43);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(20, 10);
        //send_log_window_message(QString::number(try_count) + ": 0x10 0x43 response: " + parse_message_to_hex(received), true, true);
        //qDebug() << try_count << ": 0x10 0x43 response:" << parse_message_to_hex(received);
        while (received != "")
        {
            if (received != "")
                connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x10 0x43 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x10 0x43 response:" << parse_message_to_hex(received);
            received = serial->read_serial_data(20, 10);
        }
        try_count++;
        delay(try_timeout);
    }
    connected = false;
    try_count = 0;
    output[4] = ((uint8_t)0x09);
    output[5] = ((uint8_t)0x02);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(20, 10);
        //send_log_window_message(QString::number(try_count) + ": 0x09 0x02 response: " + parse_message_to_hex(received) + " " + received, true, true);
        //qDebug() << try_count << ": 0x09 0x02 response:" << parse_message_to_hex(received);
        received.clear();
        received.append((uint8_t)0x00);
        received.append((uint8_t)0x00);
        received.append((uint8_t)0x07);
        received.append((uint8_t)0xe8);
        received.append((uint8_t)0x49);
        received.append((uint8_t)0x02);
        received.append((uint8_t)0x01);
        received.append((uint8_t)0x4a);
        received.append((uint8_t)0x46);
        received.append((uint8_t)0x31);
        received.append((uint8_t)0x56);
        received.append((uint8_t)0x41);
        received.append((uint8_t)0x32);
        received.append((uint8_t)0x4d);
        received.append((uint8_t)0x36);
        received.append((uint8_t)0x39);
        received.append((uint8_t)0x4a);
        received.append((uint8_t)0x39);
        received.append((uint8_t)0x38);
        received.append((uint8_t)0x30);
        received.append((uint8_t)0x38);
        received.append((uint8_t)0x39);
        received.append((uint8_t)0x34);
        received.append((uint8_t)0x35);
        while (received != "")
        {
            if (received != "")
                connected = true;
            QString msg = QString::fromStdString(received.remove(0, 7).toStdString());
            send_log_window_message(QString::number(try_count) + ": 0x09 0x02 response: " + parse_message_to_hex(received), true, true);
            if (received != "")
                send_log_window_message("VIN: " + msg, true, true);
            qDebug() << try_count << ": 0x09 0x02 response:" << parse_message_to_hex(received) << received;
            received = serial->read_serial_data(20, 10);
        }
        try_count++;
        delay(try_timeout);
    }
    connected = false;
    try_count = 0;
    output[4] = ((uint8_t)0x09);
    output[5] = ((uint8_t)0x06);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(20, 10);
        //send_log_window_message(QString::number(try_count) + ": 0x09 0x06 response: " + parse_message_to_hex(received), true, true);
        //qDebug() << try_count << ": 0x09 0x06 response:" << parse_message_to_hex(received);
        while (received != "")
        {
            if (received != "")
                connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x09 0x06 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x09 0x06 response:" << parse_message_to_hex(received);
            received = serial->read_serial_data(20, 10);
        }
        try_count++;
        delay(try_timeout);
    }
    connected = false;
    try_count = 0;
    output[4] = ((uint8_t)0x27);
    output[5] = ((uint8_t)0x01);
    while (try_count < 6 && connected == false)
    {
        serial->write_serial_data_echo_check(output);
        send_log_window_message("Sent: " + parse_message_to_hex(output), true, true);
        qDebug() << "Sent:" << parse_message_to_hex(output);
        delay(200);
        received = serial->read_serial_data(20, 10);
        //send_log_window_message(QString::number(try_count) + ": 0x27 0x01 response: " + parse_message_to_hex(received), true, true);
        //qDebug() << try_count << ": 0x27 0x01 response:" << parse_message_to_hex(received);
        while (received != "")
        {
            if (received != "")
                connected = true;
            send_log_window_message(QString::number(try_count) + ": 0x27 0x01 response: " + parse_message_to_hex(received), true, true);
            qDebug() << try_count << ": 0x27 0x01 response:" << parse_message_to_hex(received);
            received = serial->read_serial_data(20, 10);
        }
        try_count++;
        delay(try_timeout);
    }
    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_denso_can_recovery_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    //if (connect_bootloader_start_countdown(bootloader_start_countdown))
    //    return STATUS_ERROR;

    send_log_window_message("Start sending connection requests to Denso CAN bootloader (CAN)", true, true);
    qDebug() << "Start sending connection requests to Denso CAN bootloader (CAN)";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)((SID_ENTER_BL_CAN >> 8) & 0xFF));
    output.append((uint8_t)(SID_ENTER_BL_CAN & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    received.clear();
    int pass = 0;
    int timeout = 10000;

    serial->send_periodic_j2534_data(output, recovery_timeout);
    for (int i = 0; i < 100; i++)
    {
        if (kill_process)
            return STATUS_ERROR;
        ui->progressbar->setValue(i);
        delay(timeout / 100.0);
    }
    ui->progressbar->setValue(100);
    serial->stop_periodic_j2534_data();
    delay(200);
    received = serial->read_serial_data(20, 50);

    send_log_window_message("0xFF 0x86 response: " + parse_message_to_hex(received), true, true);
    qDebug() << "0xFF 0x86 response:" << parse_message_to_hex(received);

    send_log_window_message("Checking if bootloader started...", true, true);
    qDebug() << "Checking if bootloader started...";

    output[4] = (uint8_t)SID_START_COMM_CAN;
    output[5] = (uint8_t)(SID_CHECK_COMM_BL_CAN & 0xFF);
    output[6] = (uint8_t)0x00;
    output[7] = (uint8_t)0x00;
    output[8] = (uint8_t)0x00;
    output[9] = (uint8_t)0x00;
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    send_log_window_message("0x7A 0x90 response: " + parse_message_to_hex(received), true, true);
    qDebug() << "0x7A 0x90 response:" << parse_message_to_hex(received);
    if ((uint8_t)(received.at(1) & 0xF8) == 0x90)
    {
        send_log_window_message("Connected to bootloader, start kernel upload", true, true);
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::connect_bootloader_subaru_denso_iso15765_recovery_32bit()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    send_log_window_message("Start sending connection requests to Denso CAN bootloader (iso15765)", true, true);
    qDebug() << "Start sending connection requests to Denso CAN bootloader (iso15765)";

    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)((SID_ENTER_BL_CAN >> 8) & 0xFF));
    output.append((uint8_t)(SID_ENTER_BL_CAN & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);

    received.clear();
    int pass = 0;
    int timeout = 10000;

    serial->send_periodic_j2534_data(output, recovery_timeout);
    for (int i = 0; i < 100; i++)
    {
        ui->progressbar->setValue(i);
        delay(timeout / 100.0);
    }
    ui->progressbar->setValue(100);
    serial->stop_periodic_j2534_data();
    delay(200);
    serial->clear_tx_buffer();
    serial->clear_rx_buffer();
    //received = serial->read_serial_data(20, 50);
    //send_log_window_message("0xFF 0x86 response: " + parse_message_to_hex(received), true, true);
    //qDebug() << "0xFF 0x86 response:" << parse_message_to_hex(received);

    send_log_window_message("Checking if bootloader started...", true, true);
    qDebug() << "Checking if bootloader started...";

    output[4] = (uint8_t)SID_START_COMM_CAN;
    output[5] = (uint8_t)(SID_CHECK_COMM_BL_CAN & 0xFF);
    output[6] = (uint8_t)0x00;
    output[7] = (uint8_t)0x00;
    output[8] = (uint8_t)0x00;
    output[9] = (uint8_t)0x00;
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    send_log_window_message("0x7A 0x90 response: " + parse_message_to_hex(received), true, true);
    qDebug() << "0x7A 0x90 response:" << parse_message_to_hex(received);
    if ((uint8_t)(received.at(1) & 0xF8) == 0x90)
    {
        send_log_window_message("Connected to bootloader, start kernel upload", true, true);
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::initialize_read_mode_subaru_uj20_30_40_70_kline()
{
    QByteArray output;
    QByteArray received;
    QString msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    // Start countdown
    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    // Fast init (aty)
    // C1 33 F1 81

    // Init (att)
    // C1 33 F1 81

    // iso14230
    // 80 F0 10 01 BF
    //output.clear();
    //output.append((uint8_t)0xBF);
    //received = serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    //delay(50);
    //received = serial->read_serial_data(20, 1000);
    //send_log_window_message("SSM init response: " + parse_message_to_hex(received), true, true);
    //qDebug() << "SSM init response:" << parse_message_to_hex(received);

    serial->change_port_speed("4800");
    // SSM init
    received = send_subaru_sid_bf_ssm_init();
    if (received == "" || (uint8_t)received.at(4) != 0xFF)
        return STATUS_ERROR;

    received.remove(0, 8);
    received.remove(5, received.length() - 5);

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString ecuid = msg;
    send_log_window_message("ECU ID = " + ecuid, true, true);

    received = send_subaru_hitachi_sid_b8_change_baudrate_38400();
    send_log_window_message("0xB8 response: " + parse_message_to_hex(received), true, true);
    qDebug() << "0xB8 response:" << parse_message_to_hex(received);
    if (received == "" || (uint8_t)received.at(4) != 0xF8)
        return STATUS_ERROR;

    serial->change_port_speed("38400");

    output.clear();
    output.append((uint8_t)0xBF);
    received = serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    delay(50);
    received = serial->read_serial_data(20, 1000);
    send_log_window_message("SSM init response: " + parse_message_to_hex(received), true, true);
    qDebug() << "SSM init response:" << parse_message_to_hex(received);
    if (received == "")
        return STATUS_ERROR;

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::uninitialize_read_mode_subaru_uj20_30_40_70_kline()
{
    QByteArray output;
    QByteArray received;
    QString msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    received = send_subaru_hitachi_sid_b8_change_baudrate_4800();
    qDebug() << "SID_B8 received:" << parse_message_to_hex(received);
    if (received == "" || (uint8_t)received.at(4) != 0xF8)
        return STATUS_ERROR;

    //serial->change_port_speed("4800");

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::initialize_read_mode_subaru_uj70_kline()
{

    return STATUS_ERROR;
}

int EcuOperationsSubaru::initialize_read_mode_subaru_hitachi_70_kline()
{

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::initialize_read_mode_subaru_hitachi_70_can()
{

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::initialize_flash_mode_subaru_uj20_30_40_70_kline()
{
    QByteArray output;
    QByteArray received;
    QString msg;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    // Start countdown
    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    // SSM init
    received = send_subaru_sid_bf_ssm_init();
    //send_log_window_message("SID BF = " + parse_message_to_hex(received), true, true);
    if (received == "")
        return STATUS_ERROR;
    //qDebug() << "SID_BF received:" << parse_message_to_hex(received);
    received.remove(0, 8);
    received.remove(5, received.length() - 5);
    //qDebug() << "Received length:" << received.length();
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString ecuid = msg;

    QMessageBox::information(this, tr("Forester, Impreza, Legacy 2000-2002 K-Line (UJ WA12212920/128KB)"), "Forester, Impreza, Legacy 2000-2002 K-Line (UJ WA12212920/128KB) writing not yet confirmed!");
    received = send_subaru_hitachi_sid_af_enter_flash_mode(received);

    qDebug() << parse_message_to_hex(received);

    return STATUS_ERROR;
}

int EcuOperationsSubaru::initialize_flash_mode_subaru_uj70_kline()
{
    QByteArray output;
    QByteArray received;
    QString msg;

    QByteArray seed;
    QByteArray seed_key;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    // Start countdown
    if (connect_bootloader_start_countdown(bootloader_start_countdown))
        return STATUS_ERROR;

    // SSM init
    received = send_subaru_sid_bf_ssm_init();
    if (received == "" || (uint8_t)received.at(4) != 0xFF)
        return STATUS_ERROR;

    received.remove(0, 8);
    received.remove(5, received.length() - 5);

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }
    QString ecuid = msg;
    qDebug() << "ECU ID =" << ecuid;
    send_log_window_message("ECU ID = " + ecuid, true, true);

    // Start communication
    received = send_subaru_denso_sid_81_start_communication();
    send_log_window_message("SID_81 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xC1)
        return STATUS_ERROR;

    qDebug() << "Start communication ok: " << parse_message_to_hex(received);
    send_log_window_message("Start communication ok: " + parse_message_to_hex(received), true, true);

    // Request timing parameters
    received = send_subaru_denso_sid_83_request_timings();
    send_log_window_message("SID_83 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0xC3)
        return STATUS_ERROR;

    qDebug() << "Request timing parameters ok:" << parse_message_to_hex(received);
    send_log_window_message("Request timing parameters ok: " + parse_message_to_hex(received), true, true);

    // Request seed
    received = send_subaru_denso_sid_27_request_seed();
    send_log_window_message("SID_27_01 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
        return STATUS_ERROR;

    qDebug() << "Seed request ok:" << parse_message_to_hex(received);
    send_log_window_message("Seed request ok: " + parse_message_to_hex(received), true, true);

    seed.append(received.at(6));
    seed.append(received.at(7));
    seed.append(received.at(8));
    seed.append(received.at(9));

    seed_key = subaru_mitsubootloader_generate_kline_seed_key(seed);

    msg = parse_message_to_hex(seed_key);
    //qDebug() << "Calculated seed key: " + msg + ", sending to ECU";
    //send_log_window_message("Calculated seed key: " + msg + ", sending to ECU", true, true);
    qDebug() << "Sending seed key:" << parse_message_to_hex(seed_key);
    send_log_window_message("Sending seed key: " + parse_message_to_hex(seed_key), true, true);

    received = send_subaru_denso_sid_27_send_seed_key(seed_key);
    //send_log_window_message("SID_27_02 = " + parse_message_to_hex(received), true, true);
    if (received == "" || (uint8_t)received.at(4) != 0x67)
    {
        qDebug() << "Seed key error:" << parse_message_to_hex(received);
        send_log_window_message("Seed key error: " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }

    qDebug() << "Seed key ok:" << parse_message_to_hex(received);
    send_log_window_message("Seed key ok: " + parse_message_to_hex(received), true, true);

    return STATUS_ERROR;
}

int EcuOperationsSubaru::initialize_flash_mode_subaru_hitachi_70_kline()
{
    QByteArray output;
    QByteArray received;
    QString msg;

    serial->add_iso14230_header = false;

    return STATUS_ERROR;
}

int EcuOperationsSubaru::initialize_flash_mode_subaru_hitachi_70_can()
{
    QByteArray output;
    QByteArray received;
    QString msg;

    serial->add_iso14230_header = false;

    return STATUS_ERROR;
}

int EcuOperationsSubaru::upload_kernel_subaru_denso_kline_02_16bit(QString kernel)
{
    QFile file(kernel);
    QFile encrypted_kernel(kernel);

    QByteArray output;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t len = 0;
    uint8_t chk_sum = 0;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    //serial->change_port_speed("9600");
    serial->add_iso14230_header = false;

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
        return -1;
    }

    file_len = file.size();
    pl_len = file_len;
    pl_encr = file.readAll();
    len = pl_len;

    output.clear();
    output.append((uint8_t)(SID_OE_UPLOAD_KERNEL) & 0xFF);
    output.append((uint8_t)(flashdevices[mcu_type_index].rblocks->start >> 16) & 0xFF);
    output.append((uint8_t)(flashdevices[mcu_type_index].rblocks->start >> 8) & 0xFF);
    output.append((uint8_t)(len >> 16) & 0xFF);
    output.append((uint8_t)(len >> 8) & 0xFF);
    output.append((uint8_t)len & 0xFF);

    //pl_encr = sub_transform_wrx02_kernel_block();
    output.append(pl_encr);
    chk_sum = calculate_checksum(output, true);
    output.append((uint8_t)chk_sum);

    send_log_window_message("Start sending kernel... please wait...", true, true);
    received = serial->write_serial_data_echo_check(output);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    msg.clear();
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }
    if (received.length())
    {
        send_log_window_message("Message received " + QString::number(received.length()) + " bytes '" + msg + "'", true, true);
        send_log_window_message("Error on kernel upload!", true, true);
        return STATUS_ERROR;
    }
    else
        send_log_window_message("Kernel uploaded succesfully", true, true);

    serial->change_port_speed("39473");
    delay(200);
    received.clear();
    while (received == "")
    {
        //qDebug() << "Request kernel ID";
        received = ecuOperations->request_kernel_id();
        delay(500);
    }
    qDebug() << received;
    //    return STATUS_ERROR;

    delay(200);

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::upload_kernel_subaru_denso_kline_04_16bit(QString kernel)
{
    send_log_window_message("Initializing Subaru K-Line 04 16-bit kernel upload, please wait...", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    return STATUS_ERROR;
}

int EcuOperationsSubaru::upload_kernel_subaru_denso_kline_02_32bit(QString kernel)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t len = 0;
    uint32_t ram_addr = 0;
    uint8_t chk_sum = 0;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    //serial->change_port_speed("9600");
    serial->add_iso14230_header = false;

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
        return -1;
    }

    file_len = file.size();
    pl_len = (file_len + 3) & ~3;
    pl_encr = file.readAll();
    len = pl_len &= ~3;

    //send_log_window_message("File length " + QString::number(file_len) + ", payload length " + QString::number(pl_len), true, true);

    if (pl_len >= KERNEL_MAXSIZE_SUB) {
        send_log_window_message("***************** warning : large kernel detected *****************", true, true);
        send_log_window_message("That file seems way too big (%lu bytes) to be a typical kernel.", true, true);
        send_log_window_message("Trying anyway, but you might be using an invalid/corrupt file" + QString::number((unsigned long) pl_len), true, true);
    }

    if (file_len != pl_len) {
        send_log_window_message("Using " + QString::number(file_len) + " byte payload, padding with garbage to " + QString::number(pl_len) + " (" + QString::number(file_len) + ") bytes.\n", true, true);
    } else {
        send_log_window_message("Using " + QString::number(file_len) + " (" + QString::number(file_len) + ") byte payload.", true, true);
    }

    if ((uint32_t)pl_encr.size() != file_len)
    {
        send_log_window_message("File size error (" + QString::number(file_len) + "/" + QString::number(pl_encr.size()) + ")", true, true);
        return STATUS_ERROR;
    }

    //pl_encr = sub_transform_denso_02_32bit_kernel(pl_encr, (uint32_t) pl_len);

    for (uint32_t i = 0; i < len; i++) {
        pl_encr[i] = (uint8_t) (pl_encr[i] ^ 0x55) + 0x10;
        chk_sum += pl_encr[i];
    }

    send_log_window_message("Create kernel data header...", true, true);
    output.clear();
    output.append((uint8_t)SID_OE_UPLOAD_KERNEL & 0xFF);
    output.append((uint8_t)(flashdevices[mcu_type_index].rblocks->start >> 16) & 0xFF);
    output.append((uint8_t)(flashdevices[mcu_type_index].rblocks->start >> 8) & 0xFF);
    output.append((uint8_t)((len + 4) >> 16) & 0xFF);
    output.append((uint8_t)((len + 4) >> 8) & 0xFF);
    output.append((uint8_t)(len + 4) & 0xFF);
    output.append((uint8_t)((0x00 ^ 0x55) + 0x10) & 0xFF);
    output.append((uint8_t)0x00 & 0xFF);
    output.append((uint8_t)0x31 & 0xFF);
    output.append((uint8_t)0x61 & 0xFF);

    output[7] = calculate_checksum(output, true);
    output.append(pl_encr);
    chk_sum = calculate_checksum(output, true);
    output.append((uint8_t) chk_sum);

    send_log_window_message("Start sending kernel... please wait...", true, true);
    serial->write_serial_data_echo_check(output);
    received = serial->read_serial_data(100, serial_read_short_timeout);
    msg.clear();
    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }
    if (received.length())
    {
        send_log_window_message("Message received " + QString::number(received.length()) + " bytes '" + msg + "'", true, true);
        send_log_window_message("Error on kernel upload!", true, true);
        return STATUS_ERROR;
    }
    else
        send_log_window_message("Kernel uploaded succesfully", true, true);

    send_log_window_message("Kernel started, initializing...", true, true);

    serial->change_port_speed("62500");
    serial->add_iso14230_header = true;
    serial->iso14230_startbyte = 0x80;
    serial->iso14230_tester_id = 0xFC;
    serial->iso14230_target_id = 0x10;

    delay(100);

    received = ecuOperations->request_kernel_init();
    if (received == "")
    {
        qDebug() << "Kernel init NOK! No response from kernel. " + parse_message_to_hex(received);
        send_log_window_message("Kernel init NOK! No response from kernel. " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    if ((uint8_t)received.at(1) != SID_KERNEL_INIT + 0x40)
    {
        qDebug() << "Kernel init NOK! Got bad startcomm response from kernel. " + parse_message_to_hex(received);
        send_log_window_message("Kernel init NOK! Got bad startcomm response from kernel. " + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    else
    {
        send_log_window_message("Kernel init OK", true, true);
    }

    send_log_window_message("Requesting kernel ID", true, true);

    delay(100);

    received = ecuOperations->request_kernel_id();
    if (received == "")
        return STATUS_ERROR;

    received.remove(0, 2);
    send_log_window_message("Request kernel ID OK" + received, true, true);

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::upload_kernel_subaru_denso_kline_04_32bit(QString kernel)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t start_address = 0;
    uint32_t len = 0;
    QByteArray cks_bypass;
    uint8_t chk_sum = 0;

    QString mcu_name;

    start_address = flashdevices[mcu_type_index].rblocks->start;
    qDebug() << "Start address to upload kernel:" << hex << start_address;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }
    file_len = file.size();
    pl_len = (file_len + 3) & ~3;
    pl_encr = file.readAll();
    len = pl_len &= ~3;

    if (file_len != pl_len)
    {
        send_log_window_message("Using " + QString::number(file_len) + " byte payload, padding with garbage to " + QString::number(pl_len) + " (" + QString::number(file_len) + ") bytes.\n", true, true);
    }
    else
    {
        send_log_window_message("Using " + QString::number(file_len) + " (" + QString::number(file_len) + ") byte payload.", true, true);
    }
    if ((uint32_t)pl_encr.size() != file_len)
    {
        send_log_window_message("File size error (" + QString::number(file_len) + "/" + QString::number(pl_encr.size()) + ")", true, true);
        return STATUS_ERROR;
    }

    // Change port speed to upload kernel
    qDebug() << "Change port speed to 15625";
    if (serial->change_port_speed("15625"))
        return STATUS_ERROR;

    // Request upload
    qDebug() << "Send 'sid34_request upload'";
    received = send_subaru_denso_sid_34_request_upload(start_address, len);
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;

    send_log_window_message("Kernel upload request ok, uploading now, please wait...", true, true);

    qDebug() << "Encrypt kernel data before upload";
    //pl_encr = sub_encrypt_buf(pl_encr, (uint32_t) pl_len);
    pl_encr = subaru_denso_transform_32bit_payload(pl_encr, (uint32_t) pl_len);

    qDebug() << "Send 'sid36_transfer_data'";
    received = send_subaru_denso_sid_36_transferdata(start_address, pl_encr, len);
    if (received == "" || (uint8_t)received.at(4) != 0x76)
        return STATUS_ERROR;

    send_log_window_message("Kernel uploaded", true, true);

    /* sid34 requestDownload - checksum bypass put just after payload */
    qDebug() << "Send 'sid34_transfer_data' for chksum bypass";
    received = send_subaru_denso_sid_34_request_upload(start_address + len, 4);
    if (received == "" || (uint8_t)received.at(4) != 0x74)
        return STATUS_ERROR;

    //send_log_window_message("sid_34 checksum bypass ok", true, true);

    cks_bypass.append((uint8_t)0x00);
    cks_bypass.append((uint8_t)0x00);
    cks_bypass.append((uint8_t)0x5A);
    cks_bypass.append((uint8_t)0xA5);

    //qDebug() << parse_message_to_hex(sub_transform_denso_04_32bit_kernel(cks_bypass, (uint32_t) 4));
    //qDebug() << parse_message_to_hex(sub_encrypt_buf(cks_bypass, (uint32_t) 4));
    //cks_bypass = sub_encrypt_buf(cks_bypass, (uint32_t) 4);
    cks_bypass = subaru_denso_transform_32bit_payload(cks_bypass, (uint32_t) 4);

    /* sid36 transferData for checksum bypass */
    qDebug() << "Send 'sid36_transfer_data' for chksum bypass";
    received = send_subaru_denso_sid_36_transferdata(start_address + len, cks_bypass, 4);
    if (received == "" || (uint8_t)received.at(4) != 0x76)
        return STATUS_ERROR;

    //send_log_window_message("sid_36 checksum bypass ok", true, true);

    /* SID 37 TransferExit does not exist on all Subaru ROMs */

    /* RAMjump ! */
    qDebug() << "Send 'sid31_transfer_data' to jump to kernel";
    received = send_subaru_denso_sid_31_start_routine();
    if (received == "" || (uint8_t)received.at(4) != 0x71)
        return STATUS_ERROR;

    send_log_window_message("Kernel started, initializing...", true, true);

    serial->change_port_speed("62500");
    serial->add_iso14230_header = true;
    serial->iso14230_startbyte = 0x80;
    serial->iso14230_tester_id = 0xFC;
    serial->iso14230_target_id = 0x10;

    //serial->reset_connection();
    //serial->open_serial_port();

    delay(100);

    received = ecuOperations->request_kernel_init();
    //qDebug() << "Kernel init response:" << parse_message_to_hex(received);
    if (received == "")
    {
        //qDebug() << "Kernel init NOK! No response from kernel" + parse_message_to_hex(received);
        send_log_window_message("Kernel init NOK! No response from kernel" + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    if ((uint8_t)received.at(1) != SID_KERNEL_INIT + 0x40)
    {
        //qDebug() << "Kernel init NOK! Got bad startcomm response from kernel" + parse_message_to_hex(received);
        send_log_window_message("Kernel init NOK! Got bad startcomm response from kernel" + parse_message_to_hex(received), true, true);
        return STATUS_ERROR;
    }
    else
    {
        send_log_window_message("Kernel init OK", true, true);
    }

    send_log_window_message("Requesting kernel ID", true, true);

    delay(100);

    received = ecuOperations->request_kernel_id();
    if (received == "")
        return STATUS_ERROR;

    send_log_window_message("Request kernel ID OK: " + received, true, true);

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::upload_kernel_subaru_denso_can_32bit(QString kernel)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t start_address = 0;
    uint32_t end_address = 0;
    uint32_t len = 0;
    QByteArray cks_bypass;
    uint8_t chk_sum = 0;
    int blocks = 0;
    int byte_counter = 0;

    QString mcu_name;

    start_address = flashdevices[mcu_type_index].rblocks->start;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }
    file_len = file.size();
    pl_len = file_len + 6;
    pl_encr = file.readAll();
    blocks = file_len / 6;
    if((file_len % 6) != 0) blocks++;
    end_address = (start_address + (blocks * 6)) & 0xFFFFFFFF;

    send_log_window_message("Start sending kernel... please wait...", true, true);
    // Send kernel address
    output.clear();
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x0F);
    output.append((uint8_t)0xFF);
    output.append((uint8_t)0xFE);
    output.append((uint8_t)SID_START_COMM_CAN);
    output.append((uint8_t)(SID_KERNEL_ADDRESS + 0x04));
    output.append((uint8_t)((start_address >> 24) & 0xFF));
    output.append((uint8_t)((start_address >> 16) & 0xFF));
    output.append((uint8_t)((start_address >> 8) & 0xFF));
    output.append((uint8_t)(start_address & 0xFF));
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    received = serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    delay(200);
    received = serial->read_serial_data(20, 10);
    qDebug() << "0x7A 0x9C response:" << parse_message_to_hex(received);

    qDebug() << "Kernel start address:" << hex << start_address << ", blocks:" << hex << blocks << ", end address:" << hex << end_address;

    output[5] = (uint8_t)(0xA8 + 0x06);
    for(int i = 0; i < blocks; i++)
    {
        for(int j = 0; j < 6; j++){

            output[6 + j] = pl_encr.at(byte_counter + j);
            chk_sum += (pl_encr.at(byte_counter + j) & 0xFF);
            chk_sum = ((chk_sum >> 8) & 0xFF) + (chk_sum & 0xFF);

        }

        byte_counter += 6;
        received = serial->write_serial_data_echo_check(output);
        //qDebug() << "0xA8 message sent to bootloader to load kernel for block:" << i;

        delay(5);

    }

    qDebug() << "All 0xA8 messages sent, checksum:" << hex << chk_sum;

    // send 0xB0 command to check checksum
    output[5] = (uint8_t)(SID_KERNEL_CHECKSUM + 0x04);
    output[6] = (uint8_t)(((end_address + 1) >> 24) & 0xFF);
    output[7] = (uint8_t)(((end_address + 1) >> 16) & 0xFF);
    output[8] = (uint8_t)(((end_address + 1) >> 8) & 0xFF);
    output[9] = (uint8_t)((end_address + 1) & 0xFF);
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    qDebug() << "0xB0 message sent to bootloader to check checksum, waiting for response...";
    delay(200);
    received = serial->read_serial_data(20, 10);
    qDebug() << "Response to 0xB0 message:" << parse_message_to_hex(received);

    send_log_window_message("Kernel uploaded, starting kernel...", true, true);

    // send 0xA0 command to jump into kernel
    output[5] = (uint8_t)(SID_KERNEL_JUMP + 0x04);
    output[6] = (uint8_t)((end_address >> 24) & 0xFF);
    output[7] = (uint8_t)((end_address >> 16) & 0xFF);
    output[8] = (uint8_t)((end_address >> 8) & 0xFF);
    output[9] = (uint8_t)(end_address & 0xFF);
    output[10] = (uint8_t)0x00;
    output[11] = (uint8_t)0x00;
    serial->write_serial_data_echo_check(output);
    qDebug() << parse_message_to_hex(output);
    qDebug() << "0xA0 message sent to bootloader to jump into kernel";

    qDebug() << "ECU should now be running from kernel";

    received.clear();
    received = ecuOperations->request_kernel_id();
    if (received == "")
        return STATUS_ERROR;

    //qDebug() << "Request kernel ID OK: " << parse_message_to_hex(received);
    send_log_window_message("Request kernel ID OK: " + received, true, true);

    return STATUS_SUCCESS;
}

int EcuOperationsSubaru::upload_kernel_subaru_denso_iso15765_32bit(QString kernel)
{
    QFile file(kernel);

    QByteArray output;
    QByteArray payload;
    QByteArray received;
    QByteArray msg;
    QByteArray pl_encr;
    uint32_t file_len = 0;
    uint32_t pl_len = 0;
    uint32_t start_address = 0;
    uint32_t end_address = 0;
    uint32_t len = 0;
    QByteArray cks_bypass;
    uint8_t chk_sum = 0;
    int blocks = 0;
    int byte_counter = 0;

    QString mcu_name;

    start_address = flashdevices[mcu_type_index].rblocks->start;

    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    serial->add_iso14230_header = false;

    // Check kernel file
    if (!file.open(QIODevice::ReadOnly ))
    {
        send_log_window_message("Unable to open kernel file for reading", true, true);
        return STATUS_ERROR;
    }
    file_len = file.size();
    pl_len = file_len + 6;
    pl_encr = file.readAll();
    blocks = file_len / 6;
    if((file_len % 6) != 0) blocks++;
    end_address = (start_address + (blocks * 6)) & 0xFFFFFFFF;

    return STATUS_ERROR;
}

int EcuOperationsSubaru::read_rom_subaru_denso_kline_02_16bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    bool success = ecuOperations->read_mem_16bit_kline(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);

    return success;
}

int EcuOperationsSubaru::read_rom_subaru_denso_kline_04_16bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    send_log_window_message("Reading ROM from Subaru K-Line 04 16-bit", true, true);
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    return STATUS_ERROR;
}

int EcuOperationsSubaru::read_rom_subaru_denso_kline_32bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    bool success = ecuOperations->read_mem_32bit_kline(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);

    return success;
}

int EcuOperationsSubaru::read_rom_subaru_denso_can_32bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    bool success = ecuOperations->read_mem_32bit_can(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);

    return success;
}

int EcuOperationsSubaru::read_rom_subaru_denso_iso15765_32bit(FileActions::EcuCalDefStructure *ecuCalDef)
{
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    bool success = ecuOperations->read_mem_32bit_iso15765(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);

    return success;
}

int EcuOperationsSubaru::read_rom_subaru_uj20_30_40_70_kline(FileActions::EcuCalDefStructure *ecuCalDef)
{
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    qDebug() << mcu_type_string << mcu_type_index;
    bool success = ecuOperations->read_mem_uj20_30_40_70_kline(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);

    return success;
}

int EcuOperationsSubaru::read_rom_subaru_uj70_kline(FileActions::EcuCalDefStructure *ecuCalDef)
{
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    bool success = ecuOperations->read_mem_uj20_30_40_70_kline(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);

    return success;
}

int EcuOperationsSubaru::read_rom_subaru_hitachi_70_kline(FileActions::EcuCalDefStructure *ecuCalDef)
{
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    bool success = ecuOperations->read_mem_uj20_30_40_70_kline(ecuCalDef, flashdevices[mcu_type_index].fblocks[0].start, flashdevices[mcu_type_index].romsize);

    return success;
}

int EcuOperationsSubaru::read_rom_subaru_hitachi_70_can(FileActions::EcuCalDefStructure *ecuCalDef)
{
    bool success = false;

    return success;
}

int EcuOperationsSubaru::write_rom_subaru_denso_kline_02_16bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    if (!serial->is_serial_port_open())
    {
        send_log_window_message("ERROR: Serial port is not open.", true, true);
        return STATUS_ERROR;
    }

    bool success = ecuOperations->write_mem_16bit_kline(ecuCalDef, test_write);

    return success;
}

int EcuOperationsSubaru::write_rom_subaru_denso_kline_04_16bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    send_log_window_message("Writing ROM to Subaru K-Line 04 16-bit", true, true);

    return STATUS_ERROR;
}

int EcuOperationsSubaru::write_rom_subaru_denso_kline_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    bool success = ecuOperations->write_mem_32bit_kline(ecuCalDef, test_write);

    return success;
}

int EcuOperationsSubaru::write_rom_subaru_denso_can_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    bool success = ecuOperations->write_mem_32bit_can(ecuCalDef, test_write);

    return success;
}

int EcuOperationsSubaru::write_rom_subaru_denso_iso15765_32bit(FileActions::EcuCalDefStructure *ecuCalDef, bool test_write)
{
    bool success = ecuOperations->write_mem_32bit_iso15765(ecuCalDef, test_write);

    return success;
}

int EcuOperationsSubaru::write_rom_subaru_uj20_30_40_70_kline(FileActions::EcuCalDefStructure *ecuCalDef)
{
    bool success = false;

    return success;
}

int EcuOperationsSubaru::write_rom_subaru_uj70_kline(FileActions::EcuCalDefStructure *ecuCalDef)
{
    bool success = false;

    return success;
}

int EcuOperationsSubaru::write_rom_subaru_hitachi_70_kline(FileActions::EcuCalDefStructure *ecuCalDef)
{
    bool success = false;

    return success;
}

int EcuOperationsSubaru::write_rom_subaru_hitachi_70_can(FileActions::EcuCalDefStructure *ecuCalDef)
{
    bool success = false;

    return success;
}

/*
 * Read ECU ID
 *
 * @return ECU ID and capabilities
 */
QByteArray EcuOperationsSubaru::send_subaru_sid_a8_read_mem()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    //qDebug() << "SID 0xA8";

    send_log_window_message("Read ECU ID", true, true);
    //qDebug() << "Read ECU ID";
    output.append((uint8_t)0xA8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x02);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x03);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x04);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x05);

    serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    received = serial->read_serial_data(100, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next A8 loop";
        //qDebug() << "A8 received:" << parse_message_to_hex(received);
        send_log_window_message("Read ECU ID", true, true);
        //qDebug() << "Read ECU ID";
        serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(100, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
        //qDebug() << "0xA8 loop_cnt:" << loop_cnt;

    //qDebug() << "A8 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * ECU init
 *
 * @return ECU ID and capabilities
 */
QByteArray EcuOperationsSubaru::send_subaru_sid_bf_ssm_init()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start BF";
    send_log_window_message("SSM init", true, true);
    //qDebug() << "SSM init";
    output.append((uint8_t)0xBF);
    serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    delay(250);
    received = serial->read_serial_data(100, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        qDebug() << "Next BF loop";
        send_log_window_message("SSM init", true, true);
        //qDebug() << "SSM init";
        qDebug() << "SSM init" << parse_message_to_hex(received);
        serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(100, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0xBF loop_cnt:" << loop_cnt;

    //qDebug() << "BF received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray EcuOperationsSubaru::send_subaru_denso_sid_81_start_communication()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 81";
    output.append((uint8_t)0x81);
    serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    received = serial->read_serial_data(8, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 81 loop";
        serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(8, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x81 loop_cnt:" << loop_cnt;

    //qDebug() << "81 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Start diagnostic connection
 *
 * @return received response
 */
QByteArray EcuOperationsSubaru::send_subaru_denso_sid_83_request_timings()
{
    QByteArray output;
    QByteArray received;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 83";
    output.append((uint8_t)0x83);
    output.append((uint8_t)0x00);
    serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    received = serial->read_serial_data(12, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 83 loop";
        serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(12, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x83 loop_cnt:" << loop_cnt;

    //qDebug() << "83 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Request seed
 *
 * @return seed (4 bytes)
 */
QByteArray EcuOperationsSubaru::send_subaru_denso_sid_27_request_seed()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 27 01";
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x01);
    received = serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    //qDebug() << "27 01 send received:" << parse_message_to_hex(received);
    received = serial->read_serial_data(11, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 27 01 loop";
        serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(11, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x27 0x01 loop_cnt:" << loop_cnt;

    //qDebug() << "27 01 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Send seed key
 *
 * @return received response
 */
QByteArray EcuOperationsSubaru::send_subaru_denso_sid_27_send_seed_key(QByteArray seed_key)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 27 02";
    output.append((uint8_t)0x27);
    output.append((uint8_t)0x02);
    output.append(seed_key);
    serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    received = serial->read_serial_data(8, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 27 02 loop";
        serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(8, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x27 0x02 loop_cnt:" << loop_cnt;

    //qDebug() << "27 02 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Request start diagnostic
 *
 * @return received response
 */
QByteArray EcuOperationsSubaru::send_subaru_denso_sid_10_start_diagnostic()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 10";
    output.append((uint8_t)0x10);
    output.append((uint8_t)0x85);
    output.append((uint8_t)0x02);
    serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    received = serial->read_serial_data(7, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 10 loop";
        serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(7, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x10 loop_cnt:" << loop_cnt;

    //qDebug() << "10 received:" << parse_message_to_hex(received);

    return received;
}

/*
 * Request download (kernel)
 *
 * @return received response
 */
QByteArray EcuOperationsSubaru::send_subaru_denso_sid_34_request_upload(uint32_t dataaddr, uint32_t datalen)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 34";
    output.append((uint8_t)0x34);
    output.append((uint8_t)(dataaddr >> 16) & 0xFF);
    output.append((uint8_t)(dataaddr >> 8) & 0xFF);
    output.append((uint8_t)dataaddr & 0xFF);
    output.append((uint8_t)0x04);
    output.append((uint8_t)(datalen >> 16) & 0xFF);
    output.append((uint8_t)(datalen >> 8) & 0xFF);
    output.append((uint8_t)datalen & 0xFF);

    //qDebug() << "34 message:" << parse_message_to_hex(add_ssm_header(output, false));

    serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    received = serial->read_serial_data(7, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 34 loop";
        serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(7, receive_timeout);
        loop_cnt++;
        //qDebug() << parse_message_to_hex(received);
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x34 loop_cnt:" << loop_cnt;

    //qDebug() << "34 received:" << parse_message_to_hex(received);

    return received;
}

QByteArray EcuOperationsSubaru::send_subaru_denso_sid_36_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len)
{
    QByteArray output;
    QByteArray received;
    uint32_t blockaddr = 0;
    uint16_t blockno = 0;
    uint16_t maxblocks = 0;
    uint8_t loop_cnt = 0;

    len &= ~0x03;
    if (!buf.length() || !len) {
        send_log_window_message("Error in kernel data length!", true, true);
        return NULL;
    }

    maxblocks = (len - 1) >> 7;  // number of 128 byte blocks - 1

    //qDebug() << "Kernel upload address:" << dataaddr << "(" << hex << dataaddr << ")";
    //qDebug() << "Kernel buffer length:" << buf.length() << "(" << hex << buf.length() << ")";
    //qDebug() << "Kernel file len:" << len << "(" << hex << len << ")";

    for (blockno = 0; blockno <= maxblocks; blockno++)
    {
        if (kill_process)
            return NULL;

        blockaddr = dataaddr + blockno * 128;
        output.clear();
        output.append(0x36);
        output.append((uint8_t)(blockaddr >> 16) & 0xFF);
        output.append((uint8_t)(blockaddr >> 8) & 0xFF);
        output.append((uint8_t)blockaddr & 0xFF);

        if (blockno == maxblocks)
        {
            for (uint32_t i = 0; i < len; i++)
            {
                output.append(buf.at(i + blockno * 128));
            }
        }
        else
        {
            for (int i = 0; i < 128; i++)
            {
                output.append(buf.at(i + blockno * 128));
            }
            len -= 128;
        }

        serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
        received = serial->read_serial_data(6, receive_timeout);
        while (received == "" && loop_cnt < comm_try_count)
        {
            //qDebug() << "Next 36 loop";
            serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
            delay(comm_try_timeout);
            received = serial->read_serial_data(6, receive_timeout);
            loop_cnt++;
            //qDebug() << parse_message_to_hex(received);
        }
        //if (loop_cnt > 0)
        //    qDebug() << "0x36 loop_cnt:" << loop_cnt;
    }

    //qDebug() << "36 received:" << parse_message_to_hex(received);

    return received;

}

QByteArray EcuOperationsSubaru::send_subaru_denso_sid_53_transferdata(uint32_t dataaddr, QByteArray buf, uint32_t len)
{
    QByteArray output;
    QByteArray received;
    uint32_t blockaddr = 0;
    uint16_t blockno = 0;
    uint16_t maxblocks = 0;
    uint8_t loop_cnt = 0;

    //len &= ~0x03;
    if (!buf.length() || !len) {
        send_log_window_message("Error in kernel data length!", true, true);
        return NULL;
    }

    maxblocks = (len - 1) >> 7;  // number of 128 byte blocks - 1

    //qDebug() << "Kernel upload address:" << dataaddr << "(" << hex << dataaddr << ")";
    //qDebug() << "Kernel buffer length:" << buf.length() << "(" << hex << buf.length() << ")";
    //qDebug() << "Kernel file len:" << len << "(" << hex << len << ")";

    for (blockno = 0; blockno <= maxblocks; blockno++)
    {
        if (kill_process)
            return NULL;

        blockaddr = dataaddr + blockno * 128;
        output.clear();

        if (blockno == maxblocks)
        {
            for (uint32_t i = 0; i < len; i++)
            {
                output.append(buf.at(i + blockno * 128));
            }
        }
        else
        {
            for (int i = 0; i < 128; i++)
            {
                output.append(buf.at(i + blockno * 128));
            }
            len -= 128;
        }

        serial->write_serial_data_echo_check(output);
        //received = serial->read_serial_data(6, receive_timeout);
    }

    return received;

}

QByteArray EcuOperationsSubaru::send_subaru_denso_sid_31_start_routine()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start 31";
    output.append((uint8_t)0x31);
    output.append((uint8_t)0x01);
    output.append((uint8_t)0x01);
    serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    received = serial->read_serial_data(8, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next 31 loop";
        serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
        delay(comm_try_timeout);
        received = serial->read_serial_data(8, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x31 loop_cnt:" << loop_cnt;

    //qDebug() << "31 received:" << parse_message_to_hex(received);

    return received;
}

QByteArray EcuOperationsSubaru::send_subaru_hitachi_sid_b8_change_baudrate_4800()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start B8";
    output.clear();
    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x15);
    serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    //serial->write_serial_data_echo_check(output); // No SSM header?
    delay(200);
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

QByteArray EcuOperationsSubaru::send_subaru_hitachi_sid_b8_change_baudrate_38400()
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    //qDebug() << "Start B8";
    output.clear();
    output.append((uint8_t)0xB8);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x00);
    output.append((uint8_t)0x75);
    serial->write_serial_data_echo_check(add_ssm_header_ecu(output, false));
    //serial->write_serial_data_echo_check(output); // No SSM header?
    delay(200);
    received = serial->read_serial_data(8, receive_timeout);

    return received;
}

QByteArray EcuOperationsSubaru::send_subaru_hitachi_sid_af_enter_flash_mode(QByteArray ecu_id)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    uint32_t rom_size = flashdevices[mcu_type_index].fblocks->len - 1;
    qDebug() << hex << rom_size;

    //qDebug() << "Start AF";
    output.append((uint8_t)0xAF);
    output.append((uint8_t)0x11);
    output.append((uint8_t)ecu_id.at(0)); // ECU ID [0]
    output.append((uint8_t)ecu_id.at(1)); // ECU ID [1]
    output.append((uint8_t)ecu_id.at(2)); // ECU ID [2]
    output.append((uint8_t)ecu_id.at(3)); // ECU ID [3]
    output.append((uint8_t)ecu_id.at(4)); // ECU ID [4]
    output.append((uint8_t)(rom_size >> 16) & 0xFF); // ROM size >> 24
    output.append((uint8_t)(rom_size >> 8) & 0xFF); // ROM size >> 16
    output.append((uint8_t)rom_size & 0xFF); // ROM size
    //serial->write_serial_data_echo_check(add_ssm_header(output, false));
    serial->write_serial_data_echo_check(output); // No SSM header?
    delay(500);
    received = serial->read_serial_data(8, receive_timeout);
    while (received == "" && loop_cnt < comm_try_count)
    {
        //qDebug() << "Next AF loop";
        //serial->write_serial_data_echo_check(add_ssm_header(output, false));
        serial->write_serial_data_echo_check(output); // No SSM header?
        delay(comm_try_timeout);
        received = serial->read_serial_data(8, receive_timeout);
        loop_cnt++;
    }
    //if (loop_cnt > 0)
    //    qDebug() << "0x31 loop_cnt:" << loop_cnt;

    //qDebug() << "31 received:" << parse_message_to_hex(received);

    return received;
}

QByteArray EcuOperationsSubaru::send_subaru_hitachi_sid_af_erase_memory_block(uint32_t address)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0xAF);
    output.append((uint8_t)0x31);

    return received;
}

QByteArray EcuOperationsSubaru::send_subaru_hitachi_sid_af_write_memory_block(uint32_t address, QByteArray payload)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0xAF);
    output.append((uint8_t)0x61);

    return received;
}

QByteArray EcuOperationsSubaru::send_subaru_hitachi_sid_af_write_last_memory_block(uint32_t address, QByteArray payload)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    output.append((uint8_t)0xAF);
    output.append((uint8_t)0x69);

    return received;
}

QByteArray EcuOperationsSubaru::send_subaru_hitachi_sid_a0_read_memory_block(uint32_t block_address)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    uint32_t address_offset = 0x700000;

    output.append((uint8_t)0xA8);


    return received;
}

QByteArray EcuOperationsSubaru::send_subaru_hitachi_sid_a8_read_memory_address(uint32_t address)
{
    QByteArray output;
    QByteArray received;
    QByteArray msg;
    uint8_t loop_cnt = 0;

    uint32_t address_offset = 0x700000;

    output.append((uint8_t)0xA8);


    return received;
}

/*
 * Transform denso 32bit kernel block
 *
 * @return seed key (4 bytes)
 */
QByteArray EcuOperationsSubaru::subaru_denso_transform_wrx02_kernel(unsigned char *data, int length, bool doencrypt)
{
    QByteArray encrypted;

    int decrypt[16] =
    {
        0xA, 0x5, 0x4, 0x7,
        0x6, 0x1, 0x0, 0x3,
        0x2, 0xD, 0xC, 0xF,
        0xE, 0x9, 0x8, 0xB
    };
    int encrypt[16] =
    {
        0x6, 0x5, 0x8, 0x7,
        0x2, 0x1, 0x4, 0x3,
        0xE, 0xD, 0x0, 0xF,
        0xA, 0x9, 0xC, 0xB
    };

    int offset = 0;
    int n;

    for (int i = 0; i < length; i++, data++)
    {
        if (i+offset == 2 || i+offset == 3)
            continue; // don't transform these two bytes

        n = (*data & 0x0F) ^ 0x05; // lower nybble is XORed with 0x05

        // upper nybble is transformed using above maps
        if (doencrypt)
            n += encrypt[*data >> 4] << 4;
        else
            n += decrypt[*data >> 4] << 4;

        *data = n;
    }
    return encrypted;
}

QByteArray EcuOperationsSubaru::subaru_denso_transform_wrx04_kernel(unsigned char *data, int length, bool doencrypt)
{
    QByteArray kernel_data;

    unsigned short crypto_tableA[4] =
    {
        0x7856, 0xCE22, 0xF513, 0x6E86
    };

    unsigned char crypto_tableB[32] =
    {
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8,
    };

    int i,j;
    unsigned short word1,word2,idx2,temp,key16;

    for (i = 0; i < length; i += 4)
    {

        if (doencrypt)
        {
            word2 = (*(data+0) << 8) + *(data+1);
            word1 = (*(data+2) << 8) + *(data+3);
            for (j = 1; j <= 4; j++)
            {
                idx2 = word1 ^ crypto_tableA[j-1];
                key16 =
                        crypto_tableB[(idx2 >> 0) & 0x1F]
                    + (crypto_tableB[(idx2 >> 4) & 0x1F] << 4)
                    + (crypto_tableB[(idx2 >> 8) & 0x1F] << 8)
                    + (crypto_tableB[(((idx2 & 0x1) << 4) + (idx2 >> 12)) & 0x1F] << 12);
                barrel_shift_16_right(&key16);
                barrel_shift_16_right(&key16);
                barrel_shift_16_right(&key16);

                temp = word1;
                word1 = key16 ^ word2;
                word2 = temp;

            }
            *(data+0) = word1 >> 8;
            *(data+1) = word1 & 0xFF;
            *(data+2) = word2 >> 8;
            *(data+3) = word2 & 0xFF;
        }
        else
        {
            word1 = (*(data+0) << 8) + *(data+1);
            word2 = (*(data+2) << 8) + *(data+3);
            for (j = 4; j > 0; j--)
            {
                idx2 = word2 ^ crypto_tableA[j-1];
                key16 =
                        crypto_tableB[(idx2 >> 0) & 0x1F]
                    + (crypto_tableB[(idx2 >> 4) & 0x1F] << 4)
                    + (crypto_tableB[(idx2 >> 8) & 0x1F] << 8)
                    + (crypto_tableB[(((idx2 & 0x1) << 4) + (idx2 >> 12)) & 0x1F] << 12);
                barrel_shift_16_right(&key16);
                barrel_shift_16_right(&key16);
                barrel_shift_16_right(&key16);
                temp = word2;
                word2 = key16 ^ word1;
                word1 = temp;
            }
            *(data+0) = word2 >> 8;
            *(data+1) = word2 & 0xFF;
            *(data+2) = word1 >> 8;
            *(data+3) = word1 & 0xFF;
        }
        kernel_data.append(*(data+0));
        kernel_data.append(*(data+1));
        kernel_data.append(*(data+2));
        kernel_data.append(*(data+3));
        data += 4;
    }
    return kernel_data;
}

QByteArray EcuOperationsSubaru::subaru_denso_transform_02_32bit_kernel(QByteArray buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        buf[i] = (uint8_t) (buf[i] ^ 0x55) + 0x10;
    }

    return buf;
}

QByteArray EcuOperationsSubaru::subaru_denso_transform_32bit_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
        0x7856, 0xCE22, 0xF513, 0x6E86
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    encrypted = subaru_denso_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return encrypted;
}

QByteArray EcuOperationsSubaru::subaru_denso_calculate_32bit_payload(QByteArray buf, uint32_t len, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
{
    QByteArray encrypted;
    uint32_t datatoencrypt32, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    if (!buf.length() || !len) {
        return NULL;
    }

    encrypted.clear();

    len &= ~3;
    for (uint32_t i = 0; i < len; i += 4) {
        datatoencrypt32 = ((buf.at(i) << 24) & 0xFF000000) | ((buf.at(i + 1) << 16) & 0xFF0000) | ((buf.at(i + 2) << 8) & 0xFF00) | (buf.at(i + 3) & 0xFF);

        for (ki = 0; ki < 4; ki++) {

            wordtogenerateindex = datatoencrypt32;
            wordtobeencrypted = datatoencrypt32 >> 16;
            index = wordtogenerateindex ^ keytogenerateindex[ki];
            index += index << 16;
            encryptionkey = 0;

            for (n = 0; n < 4; n++) {
                encryptionkey += indextransformation[(index >> (n * 4)) & 0x1F] << (n * 4);
            }

            encryptionkey = (encryptionkey >> 3) + (encryptionkey << 13);
            datatoencrypt32 = (encryptionkey ^ wordtobeencrypted) + (wordtogenerateindex << 16);
        }

        datatoencrypt32 = (datatoencrypt32 >> 16) + (datatoencrypt32 << 16);

        encrypted.append((datatoencrypt32 >> 24) & 0xFF);
        encrypted.append((datatoencrypt32 >> 16) & 0xFF);
        encrypted.append((datatoencrypt32 >> 8) & 0xFF);
        encrypted.append(datatoencrypt32 & 0xFF);
        //encrypted.append(sub_encrypt(tempbuf));
    }
    return encrypted;
}

/*
 * Generate seed keys from received seed bytes
 *
 * @return seed key (4 bytes)
 */
/*********************************
 * Denso K-Line ECUs seed key
 ********************************/
QByteArray EcuOperationsSubaru::subaru_denso_generate_kline_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x53DA, 0x33BC, 0x72EB, 0x437D,
        0x7CA3, 0x3382, 0x834F, 0x3608,
        0xAFB8, 0x503D, 0xDBA3, 0x9D34,
        0x3563, 0x6B70, 0x6E74, 0x88F0
    };

    const uint16_t keytogenerateindex_2[]={
        0x24B9, 0x9D91, 0xFF0C, 0xB8D5,
        0x15BB, 0xF998, 0x8723, 0x9E05,
        0x7092, 0xD683, 0xBA03, 0x59E1,
        0x6136, 0x9B9A, 0x9CFB, 0x9DDB
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/******************************
 * Denso CAN ECUs seed key
 *****************************/
QByteArray EcuOperationsSubaru::subaru_denso_generate_can_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x24B9, 0x9D91, 0xFF0C, 0xB8D5,
        0x15BB, 0xF998, 0x8723, 0x9E05,
        0x7092, 0xD683, 0xBA03, 0x59E1,
        0x6136, 0x9B9A, 0x9CFB, 0x9DDB
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/***************************************
 * EcuTek'd Denso K-Line ECUs seed key
 **************************************/
QByteArray EcuOperationsSubaru::subaru_denso_generate_ecutek_kline_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x53DA, 0x33BC, 0x72EB, 0x437D,
        0x7CA3, 0x3382, 0x834F, 0x3608,
        0xAFB8, 0x503D, 0xDBA3, 0x9D34,
        0x3563, 0x6B70, 0x6E74, 0x88F0
    };

    const uint16_t keytogenerateindex_2[]={
        0x24B9, 0x9D91, 0xFF0C, 0xB8D5,
        0x15BB, 0xF998, 0x8723, 0x9E05,
        0x7092, 0xD683, 0xBA03, 0x59E1,
        0x6136, 0x9B9A, 0x9CFB, 0x9DDB
    };

    const uint8_t indextransformation[]={
        0x4, 0x2, 0x5, 0x1, 0x8, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/************************************
 * EcuTek'd Denso CAN ECUs seed key
 ***********************************/
QByteArray EcuOperationsSubaru::subaru_denso_generate_ecutek_can_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x24B9, 0x9D91, 0xFF0C, 0xB8D5,
        0x15BB, 0xF998, 0x8723, 0x9E05,
        0x7092, 0xD683, 0xBA03, 0x59E1,
        0x6136, 0x9B9A, 0x9CFB, 0x9DDB
    };

    const uint8_t indextransformation[]={
        0x4, 0x2, 0x5, 0x1, 0x8, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

/************************************
 * COBB'd Denso CAN ECUs seed key
 ***********************************/
QByteArray EcuOperationsSubaru::subaru_denso_generate_cobb_can_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x77C1, 0x80BB, 0xD5C1, 0x7A94,
        0x11F0, 0x25FF, 0xC365, 0x10B4,
        0x48DA, 0x6720, 0x3255, 0xFA17,
        0x60BF, 0x780E, 0x9C1D, 0x5A28
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

QByteArray EcuOperationsSubaru::subaru_denso_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
{
    QByteArray key;

    uint32_t seed, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    seed = (requested_seed.at(0) << 24) & 0xFF000000;
    seed += (requested_seed.at(1) << 16) & 0x00FF0000;
    seed += (requested_seed.at(2) << 8) & 0x0000FF00;
    seed += requested_seed.at(3) & 0x000000FF;
    //seed = reconst_32(seed8);

    for (ki = 15; ki >= 0; ki--) {

        wordtogenerateindex = seed;
        wordtobeencrypted = seed >> 16;
        index = wordtogenerateindex ^ keytogenerateindex[ki];
        index += index << 16;
        encryptionkey = 0;

        for (n = 0; n < 4; n++) {
            encryptionkey += indextransformation[(index >> (n * 4)) & 0x1F] << (n * 4);
        }

        encryptionkey = (encryptionkey >> 3) + (encryptionkey << 13);
        seed = (encryptionkey ^ wordtobeencrypted) + (wordtogenerateindex << 16);
    }

    seed = (seed >> 16) + (seed << 16);

    key.clear();
    key.append((uint8_t)(seed >> 24));
    key.append((uint8_t)(seed >> 16));
    key.append((uint8_t)(seed >> 8));
    key.append((uint8_t)seed);

    //write_32b(seed, key);

    return key;
}

QByteArray EcuOperationsSubaru::subaru_hitachi_transform_32bit_payload(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    const uint16_t keytogenerateindex[]={
        0xF50E, 0x973C, 0x77F4, 0x14CA
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    encrypted = subaru_denso_calculate_32bit_payload(buf, len, keytogenerateindex, indextransformation);

    return encrypted;
}

/***********************************
 * Hitachi K-Line ECUs seed key
 **********************************/
QByteArray EcuOperationsSubaru::subaru_hitachi_generate_kline_seed_key_1(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x24B9, 0x9D91, 0xFF0C, 0xB8D5,
        0x15BB, 0xF998, 0x8723, 0x9E05,
        0x7092, 0xD683, 0xBA03, 0x59E1,
        0x6136, 0x9B9A, 0x9CFB, 0x9DDB
    };

    const uint16_t keytogenerateindex_2[]={
        0x3275, 0x6AD8, 0x1062, 0x512B,
        0xD695, 0x7640, 0x25F6, 0xAC45,
        0x6803, 0xE5DA, 0xC821, 0x36BF,
        0xA433, 0x3F41, 0x842C, 0x05D9
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

QByteArray EcuOperationsSubaru::subaru_hitachi_generate_kline_seed_key_2(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x24B9, 0x9D91, 0xFF0C, 0xB8D5,
        0x15BB, 0xF998, 0x8723, 0x9E05,
        0x7092, 0xD683, 0xBA03, 0x59E1,
        0x6136, 0x9B9A, 0x9CFB, 0x9DDB
    };

    const uint16_t keytogenerateindex_2[]={
        0x3275, 0x6AD8, 0x1062, 0x512B,
        0xD695, 0x7640, 0x25F6, 0xAC45,
        0x6803, 0xE5DA, 0xC821, 0x36BF,
        0xA433, 0x3F41, 0x842C, 0x05D9
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_2, indextransformation);

    return key;
}

/********************************
 * Hitachi CAN ECUs seed key
 *******************************/
QByteArray EcuOperationsSubaru::subaru_hitachi_generate_can_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={ // Hitachi/Denso CAN and K-Line ECU byte_1
        0x24B9, 0x9D91, 0xFF0C, 0xB8D5,
        0x15BB, 0xF998, 0x8723, 0x9E05,
        0x7092, 0xD683, 0xBA03, 0x59E1,
        0x6136, 0x9B9A, 0x9CFB, 0x9DDB
    };

    const uint16_t keytogenerateindex_2[]={ // Hitachi/Denso CAN ECU byte_2
        0x90A1, 0x2F92, 0xDE3C, 0xCDC0,
        0x1A99, 0x437C, 0xF91B, 0xDB57,
        0x96BA, 0xDE10, 0xFCAF, 0x3F31,
        0xF47F, 0x0BB6, 0x16E9, 0x4645
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_2, indextransformation);

    return key;
}

QByteArray EcuOperationsSubaru::subaru_mitsubootloader_generate_kline_seed_key(QByteArray requested_seed)
{
    QByteArray key;

    const uint16_t keytogenerateindex_1[]={
        0x8519, 0x5c53, 0xc0e9, 0x2452,
        0x1e68, 0x6feb, 0x2648, 0x81e2,
        0x8ce4, 0x953b, 0x1ca9, 0x6180,
        0xb85e, 0x5109, 0xdb3c, 0x3cf2,
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    key = subaru_denso_calculate_seed_key(requested_seed, keytogenerateindex_1, indextransformation);

    return key;
}

QByteArray EcuOperationsSubaru::subaru_hitachi_calculate_seed_key(QByteArray requested_seed, const uint16_t *keytogenerateindex, const uint8_t *indextransformation)
{
    QByteArray key;

    return key;
}

QByteArray EcuOperationsSubaru::subaru_denso_encrypt_buf(QByteArray buf, uint32_t len)
{
    QByteArray encrypted;

    if (!buf.length() || !len) {
        return NULL;
    }

    len &= ~3;
    for (uint32_t i = 0; i < len; i += 4) {
        uint8_t tempbuf[4];
        tempbuf[0] = buf.at(i);
        tempbuf[1] = buf.at(i + 1);
        tempbuf[2] = buf.at(i + 2);
        tempbuf[3] = buf.at(i + 3);
        //memcpy(tempbuf, buf, 4);
        encrypted.append(subaru_denso_encrypt(tempbuf));
        //buf += 4;
    }
    return encrypted;
}

/** For Subaru, encrypts data for upload
 * writes 4 bytes in buffer *encrypteddata
 */
QByteArray EcuOperationsSubaru::subaru_denso_encrypt(const uint8_t *datatoencrypt)
{
    QByteArray encrypted;

    uint32_t datatoencrypt32, index;
    uint16_t wordtogenerateindex, wordtobeencrypted, encryptionkey;
    int ki, n;

    const uint16_t keytogenerateindex[]={
        0x7856, 0xCE22, 0xF513, 0x6E86
    };

    const uint8_t indextransformation[]={
        0x5, 0x6, 0x7, 0x1, 0x9, 0xC, 0xD, 0x8,
        0xA, 0xD, 0x2, 0xB, 0xF, 0x4, 0x0, 0x3,
        0xB, 0x4, 0x6, 0x0, 0xF, 0x2, 0xD, 0x9,
        0x5, 0xC, 0x1, 0xA, 0x3, 0xD, 0xE, 0x8
    };

    datatoencrypt32 = datatoencrypt[0] << 24 |datatoencrypt[1] << 16 |datatoencrypt[2] << 8 | datatoencrypt[3];

    for (ki = 0; ki < 4; ki++) {

        wordtogenerateindex = datatoencrypt32;
        wordtobeencrypted = datatoencrypt32 >> 16;
        index = wordtogenerateindex ^ keytogenerateindex[ki];
        index += index << 16;
        encryptionkey = 0;

        for (n = 0; n < 4; n++) {
            encryptionkey += indextransformation[(index >> (n * 4)) & 0x1F] << (n * 4);
        }

        encryptionkey = (encryptionkey >> 3) + (encryptionkey << 13);
        datatoencrypt32 = (encryptionkey ^ wordtobeencrypted) + (wordtogenerateindex << 16);
    }

    datatoencrypt32 = (datatoencrypt32 >> 16) + (datatoencrypt32 << 16);

    encrypted.append((datatoencrypt32 >> 24) & 0xFF);
    encrypted.append((datatoencrypt32 >> 16) & 0xFF);
    encrypted.append((datatoencrypt32 >> 8) & 0xFF);
    encrypted.append(datatoencrypt32 & 0xFF);

    return encrypted;
}

QByteArray EcuOperationsSubaru::add_ssm_header_ecu(QByteArray output, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, (uint8_t)0x10);
    output.insert(2, (uint8_t)0xF0);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    //qDebug() << "Generated SSM message:" << parse_message_to_hex(output);

    return output;
}

QByteArray EcuOperationsSubaru::add_ssm_header_tcu(QByteArray output, bool dec_0x100)
{
    uint8_t length = output.length();

    output.insert(0, (uint8_t)0x80);
    output.insert(1, (uint8_t)0x18);
    output.insert(2, (uint8_t)0xF0);
    output.insert(3, length);

    output.append(calculate_checksum(output, dec_0x100));

    //qDebug() << "Generated SSM message:" << parse_message_to_hex(output);

    return output;
}

int EcuOperationsSubaru::check_received_message(QByteArray msg, QByteArray received)
{
    for (int i = 0; i < msg.length(); i++)
    {
        if (received.length() > i - 1)
            if (msg.at(i) != received.at(i))
                return 0;
    }

    return 1;
}

int EcuOperationsSubaru::connect_bootloader_start_countdown(int timeout)
{
    for (int i = timeout; i > 0; i--)
    {
        if (kill_process)
            break;
        send_log_window_message("Start in " + QString::number(i), true, true);
        //qDebug() << "Countdown:" << i;
        delay(1000);
    }
    if (!kill_process)
    {
        send_log_window_message("Turn ignition on NOW!", true, true);
        delay(500);
        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}


void EcuOperationsSubaru::barrel_shift_16_right(unsigned short *barrel)
{
    if (*barrel & 1)
        *barrel = (*barrel >> 1) + 0x8000;
    else
        *barrel = *barrel >> 1;
}


uint8_t EcuOperationsSubaru::calculate_checksum(QByteArray output, bool dec_0x100)
{
    uint8_t checksum = 0;

    for (uint16_t i = 0; i < output.length(); i++)
        checksum += (uint8_t)output.at(i);

    if (dec_0x100)
        checksum = (uint8_t) (0x100 - checksum);

    return checksum;
}

QString EcuOperationsSubaru::parse_message_to_hex(QByteArray received)
{
    QByteArray msg;

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

int EcuOperationsSubaru::send_log_window_message(QString message, bool timestamp, bool linefeed)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']  ");

    if (timestamp)
        message = dateTimeString + message;
    if (linefeed)
        message = message + "\n";

    QTextEdit* textedit = this->findChild<QTextEdit*>("text_edit");
    if (textedit)
    {
        ui->text_edit->insertPlainText(message);
        ui->text_edit->ensureCursorVisible();

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

void EcuOperationsSubaru::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}

