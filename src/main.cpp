#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <functional>
#include <iostream>
#include <chrono>
#include "sonia_common_cpp/SerialConn.h"
#include "dropper_test/SharedQueue.h"


class Tester
{
public:
    Tester(sonia_common_cpp::SerialConn *sc)
    {
        _sc = sc;
        printf("Port status: %i\n", _sc->OpenPort());
        _sc->Flush();
        reader = std::thread(std::bind(&Tester::readData, this));
        parser = std::thread(std::bind(&Tester::parseData, this));
    }

private:
    void readData()
    {
        uint8_t data[8192];
        while (1)
        {
            // std::this_thread::sleep_for(std::chrono::milliseconds(300));

            ssize_t str_len = _sc->ReadPackets(8192, data);

            if (str_len != -1)
            {
                for (ssize_t i = 0; i < 8192; i++)
                {
                    parseQueue.push_back(data[i]);
                }
            }
        }
    }

    void parseData()
    {
        while (1)
        {
            // std::this_thread::sleep_for(std::chrono::milliseconds(300));
            // read until the start there or the queue is empty
            while (!parseQueue.empty())
            {
                if (parseQueue.front() != 0x3A)
                {
                    parseQueue.pop_front();
                }
                else
                {
                    // sonia_common::SendRS485Msg msg = sonia_common::SendRS485Msg();

                    // pop the unused start data
                    parseQueue.pop_front();

                    uint8_t slave = parseQueue.get_n_pop_front();
                    uint8_t cmd = parseQueue.get_n_pop_front();
                    unsigned char nbByte = parseQueue.get_n_pop_front();

                    for (int i = 0; i < nbByte; i++)
                    {
                        parseQueue.get_n_pop_front();
                    }

                    uint16_t checksum = (uint16_t)(parseQueue.get_n_pop_front() << 8);
                    checksum += parseQueue.get_n_pop_front();

                    // pop the unused end data
                    parseQueue.pop_front();

                    // uint16_t calc_checksum = calculateCheckSum(msg.slave, msg.cmd, nbByte, msg.data);

                    // if the checksum is bad, drop the packet
                    // if (checksum == calc_checksum)
                    // {
                    //     publisher.publish(msg);
                    // }
                    if (slave == 0 || slave == 1 || slave == 2 || slave == 3){
                        continue;
                    }
                    std::cout << "Slave: " << (int)slave << ", ";
                    std::cout << "CMD: " << (int)cmd << ", " << std::endl;

                }
            }
        }
    }
    sonia_common_cpp::SerialConn *_sc;
    std::thread reader;
    std::thread parser;

    SharedQueue<uint8_t> parseQueue;
};

int main()
{
    sonia_common_cpp::SerialConn *sc = new sonia_common_cpp::SerialConn("/dev/RS485", B115200, false);
    Tester tester(sc);
    const uint8_t get_kill_status[8] = {0x3A, 4, 1, 1, 0, 0, 77, 0x0D};
    const uint8_t get_mission_status[8] = {0x3A, 4, 0, 1, 1, 0, 77, 0x0D};
    while (1)
    {
        // sc->Transmit(get_mission_status, 8);
        // std::this_thread::sleep_for(std::chrono::milliseconds(300));
        sc->Flush();
        sc->Transmit(get_kill_status, 8);
        printf("POLL\n");
        sleep(1);
    }

    return 0;
}
