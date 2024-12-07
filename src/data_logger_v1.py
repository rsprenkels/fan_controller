from typing import Optional, Annotated

from fastapi import FastAPI, Request, Response, Header
from pydantic import BaseModel, Field, NaiveDatetime
from datetime import date, datetime, time, timedelta
import logging as log

log.basicConfig(format='%(asctime)s - %(message)s', level=log.INFO)

from fastapi import FastAPI, Depends, Request
import time
from data_dao import dao

app = FastAPI()

async def get_body(request: Request):
    return await request.body()

@app.post("/log_1")
def input_request(body: bytes = Depends(get_body)):
    act_body = body.decode("unicode_escape")
    key, value = list(act_body.split('\t'))
    log.info(f"New request arrived: key: {key} value: {value}")
    dao.add(datetime.now(), k=key, value=value)
    log.info(f'{dao.data}')
    return body

# start the application from a commandline prompt like this:
# fastapi dev data_logger.py