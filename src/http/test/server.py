from fastapi import FastAPI
from fastapi.responses import JSONResponse
from pydantic import BaseModel
import uvicorn
import asyncio

class HelloRequest(BaseModel):
    name: str


app = FastAPI()


@app.get("/hello")
def read_root():
    return JSONResponse(content={"Hello": "World"})

@app.post("/hello")
async def read_root(json_data: HelloRequest):
    print(json_data.name)
    # 模拟异步操作
    # await asyncio.sleep(10)
    return JSONResponse(content=json_data.model_dump())


if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8989)
