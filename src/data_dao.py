import os
from datetime import datetime
import sqlite3
from time import strftime
import logging as log
log.basicConfig(format='%(asctime)s - %(message)s', level=log.INFO)

db_fn_default = 'data.sqlite'

class Dao():
    def __init__(self, db_file: str = db_fn_default):
        if os.path.isfile(db_file):
            self.conn = sqlite3.connect(db_file, check_same_thread=False)
            print(f'database connected {db_file}')
        else:
            self.conn = sqlite3.connect(db_file, check_same_thread=False)
            self.conn.execute('''
                create table data (
                    dt int,
                    k char(20),
                    value text);
            ''')
            print(f'database created {db_file}')

    def add(self, k: str, value: str):
        statement = f"insert into data (dt, k, value) VALUES (strftime('%s','now'), '{k}', '{value}')"
        print(f'executing {statement}')
        self.conn.execute(statement)
        self.conn.commit()


def remove_if_exists(filename):
    if os.path.isfile(filename):
        os.remove(filename)

# https://gist.github.com/pdc/1188720 for mocking time

def test_1():
    test_db = 'test1_dbfile.sqlite'
    remove_if_exists(test_db)
    d = Dao(test_db)
    d.add('test1key', 'test1_value')

