from datetime import date, datetime, time, timedelta
import logging as log

from pydantic import BaseModel
from typing import List

log.basicConfig(format='%(asctime)s - %(message)s', level=log.INFO)

from fastapi import FastAPI, Depends, Request
from data_dao import Dao

app = FastAPI()
dao = Dao()

class KeyValue(BaseModel):
    k: str
    v: str

@app.post("/log")
def input_request(kv_list: List[KeyValue]):
    log.info(f"New request arrived: {kv_list}")
    for kv in kv_list:
        dao.add(kv.k, kv.v)

# start the application from a commandline prompt like this:
# fastapi dev data_logger.py