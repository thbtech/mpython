#!/usr/bin/env python
"""Generates the pin_cap table file for the SAMD port."""

from __future__ import print_function

import argparse
import sys
import csv

table_header = """// This file was automatically generated by make-pin-cap.py
//

#define PA(pin)  (pin)
#define PB(pin)  32 + (pin)
#define PC(pin)  64 + (pin)
#define PD(pin)  96 + (pin)

"""


class Pins:
    def __init__(self):
        self.board_pins = []  # list of pin objects

    def parse_csv_file(self, filename):
        with open(filename, "r") as csvfile:
            rows = csv.reader(csvfile)
            for row in rows:
                # Pin numbers must start with "PA", "PB", "PC" or "PD"
                if len(row) > 0 and row[0].strip().upper()[:2] in ("PA", "PB", "PC", "PD"):
                    self.board_pins.append(row)

    def print_table(self, table_filename, mcu_name):
        with open(table_filename, "wt") as table_file:
            table_file.write(table_header)
            table_file.write("const pin_cap_t pin_cap_table[] = {\n")
            if mcu_name == "SAMD21":
                for row in self.board_pins:
                    pin = (
                        row[0][0:2].upper()
                        + "("
                        + (row[0][2:] if row[0][2] != "0" else row[0][3:])
                        + ")"
                    )
                    eic = row[1] if row[1] else "0xff"
                    adc = row[2] if row[2] else "0xff"
                    table_file.write("    {%s, %s, %s" % (pin, eic, adc))
                    for cell in row[3:]:
                        if cell:
                            table_file.write(
                                ", 0x%s" % cell if len(cell) == 2 else ", 0x0%s" % cell
                            )
                        else:
                            table_file.write(", 0xff")
                    table_file.write("},\n")
            else:
                for row in self.board_pins:
                    pin = (
                        row[0][0:2].upper()
                        + "("
                        + (row[0][2:] if row[0][2] != "0" else row[0][3:])
                        + ")"
                    )
                    eic = row[1] if row[1] else "0xff"
                    adc0 = row[2] if row[2] else "0xff"
                    adc1 = row[3] if row[3] else "0xff"
                    table_file.write("    {%s, %s, %s, %s" % (pin, eic, adc0, adc1))
                    for cell in row[4:]:
                        if cell:
                            table_file.write(
                                ", 0x%s" % cell if len(cell) == 2 else ", 0x0%s" % cell
                            )
                        else:
                            table_file.write(", 0xff")
                    table_file.write("},\n")
            table_file.write("};\n")


def main():
    parser = argparse.ArgumentParser(
        prog="make-pin-cap.py",
        usage="%(prog)s [options] [command]",
        description="Generate MCU-specific pin cap table file",
    )
    parser.add_argument(
        "-c",
        "--csv",
        dest="csv_filename",
        help="Specifies the pin-mux-xxxx.csv filename",
    )
    parser.add_argument(
        "-t",
        "--table",
        dest="table_filename",
        help="Specifies the name of the generated pin cap table file",
    )
    parser.add_argument(
        "-m",
        "--mcu",
        dest="mcu_name",
        help="Specifies type of the MCU (SAMD21 or SAMD51)",
    )
    args = parser.parse_args(sys.argv[1:])

    pins = Pins()

    if args.csv_filename:
        pins.parse_csv_file(args.csv_filename)

    if args.table_filename:
        pins.print_table(args.table_filename, args.mcu_name)


if __name__ == "__main__":
    main()
