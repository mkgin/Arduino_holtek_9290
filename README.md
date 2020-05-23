### README.md

Messing about with a HOLTEK HT9290A chip.

## links

* https://hackaday.io/project/171885-control-a-obsolete-holtek-ht9290ab-dtmf-dialer

## todo
* control reading and writing from a µcontroller pin / button
* improve reading

## notes

* reading
 * after sending a read command, the HT9290 sometimes starts sending data back before the microcontroller has finished sending the read command.
 * it appears that when this happens, the following 5 bit chunk is identical to the unreadable one
* writing
 * maybe it only takes the first number written after powering up
 * could be that it is stuck waiting for  more data to be written
 * sensitive to number of bytes written after?

* triggering "dialing"
 * dailing stops on a read command. perhaps on other commands