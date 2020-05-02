// (c) Copyright Arduino. 2016
// Released under Apache License, version 2.0

#include "b64.h"

#include "WebSocketStream.h"

WebSocketStream::WebSocketStream(Stream& aStream)
 : HttpStream(aStream),
   iTxStarted(false),
   iRxSize(0)
{
}

int WebSocketStream::begin(const char* aPath)
{
    // start the GET request
    beginRequest();
    connectionKeepAlive();
    int status = get(aPath);

    if (status == 0)
    {
        uint8_t randomKey[16];
        char base64RandomKey[25];

        // create a random key for the connection upgrade
        for (int i = 0; i < (int)sizeof(randomKey); i++)
        {
            randomKey[i] = random(0x01, 0xff);
        }
        memset(base64RandomKey, 0x00, sizeof(base64RandomKey));
        b64_encode(randomKey, sizeof(randomKey), (unsigned char*)base64RandomKey, sizeof(base64RandomKey));

        // start the connection upgrade sequence
        sendHeader("Upgrade", "websocket");
        sendHeader("Connection", "Upgrade");
        sendHeader("Sec-WebSocket-Key", base64RandomKey);
        sendHeader("Sec-WebSocket-Version", "13");
        endRequest();

        status = responseStatusCode();

        if (status > 0)
        {
            skipResponseHeaders();
        }
    }

    iRxSize = 0;

    // status code of 101 means success
    return (status == 101) ? 0 : status;
}

int WebSocketStream::begin(const String& aPath)
{
    return begin(aPath.c_str());
}

int WebSocketStream::beginMessage(int aType)
{
    if (iTxStarted)
    {
        // fail TX already started
        return 1;
    }

    iTxStarted = true;
    iTxMessageType = (aType & 0xf);
    iTxSize = 0;

    return 0;
}

int WebSocketStream::endMessage()
{
    if (!iTxStarted)
    {
        // fail TX not started
        return 1;
    }

    // send FIN + the message type (opcode)
    HttpStream::write(0x80 | iTxMessageType);

    // the message is masked (0x80)
    // send the length
    if (iTxSize < 126)
    {
        HttpStream::write(0x80 | (uint8_t)iTxSize);
    }
    else if (iTxSize < 0xffff)
    {
        HttpStream::write(0x80 | 126);
        HttpStream::write((iTxSize >> 8) & 0xff);
        HttpStream::write((iTxSize >> 0) & 0xff);
    }
    else
    {
        HttpStream::write(0x80 | 127);
        HttpStream::write((iTxSize >> 56) & 0xff);
        HttpStream::write((iTxSize >> 48) & 0xff);
        HttpStream::write((iTxSize >> 40) & 0xff);
        HttpStream::write((iTxSize >> 32) & 0xff);
        HttpStream::write((iTxSize >> 24) & 0xff);
        HttpStream::write((iTxSize >> 16) & 0xff);
        HttpStream::write((iTxSize >> 8) & 0xff);
        HttpStream::write((iTxSize >> 0) & 0xff);
    }

    uint8_t maskKey[4];

    // create a random mask for the data and send
    for (int i = 0; i < (int)sizeof(maskKey); i++)
    {
        maskKey[i] = random(0xff);
    }
    HttpStream::write(maskKey, sizeof(maskKey));

    // mask the data and send
    for (int i = 0; i < (int)iTxSize; i++) {
        iTxBuffer[i] ^= maskKey[i % sizeof(maskKey)];
    }

    size_t txSize = iTxSize;

    iTxStarted = false;
    iTxSize = 0;

    return (HttpStream::write(iTxBuffer, txSize) == txSize) ? 0 : 1;
}

size_t WebSocketStream::write(uint8_t aByte)
{
    return write(&aByte, sizeof(aByte));
}

size_t WebSocketStream::write(const uint8_t *aBuffer, size_t aSize)
{
    if (iState < eReadingBody)
    {
        // have not upgraded the connection yet
        return HttpStream::write(aBuffer, aSize);
    }

    if (!iTxStarted)
    {
        // fail TX not   started
        return 0;
    }

    // check if the write size, fits in the buffer
    if ((iTxSize + aSize) > sizeof(iTxBuffer))
    {
        aSize = sizeof(iTxSize) - iTxSize;
    }

    // copy data into the buffer
    memcpy(iTxBuffer + iTxSize, aBuffer, aSize);

    iTxSize += aSize;
    
    return aSize;
}

int WebSocketStream::parseMessage()
{
    flushRx();

    // make sure 2 bytes (opcode + length)
    // are available
    if (HttpStream::available() < 2)
    {
        return 0;
    }

    // read open code and length
    uint8_t opcode = HttpStream::read();
    int length = HttpStream::read();

    if ((opcode & 0x0f) == 0)
    {
        // continuation, use previous opcode and update flags
        iRxOpCode |= opcode;
    }
    else
    {
        iRxOpCode = opcode;
    }

    iRxMasked = (length & 0x80);
    length &= 0x7f;

    // read the RX size
    if (length < 126)
    {
        iRxSize = length;
    }
    else if (length == 126)
    {
        iRxSize = (HttpStream::read() << 8) | HttpStream::read();
    }
    else
    {
        iRxSize = ((uint64_t)HttpStream::read() << 56) | 
                    ((uint64_t)HttpStream::read() << 48) | 
                    ((uint64_t)HttpStream::read() << 40) | 
                    ((uint64_t)HttpStream::read() << 32) | 
                    ((uint64_t)HttpStream::read() << 24) | 
                    ((uint64_t)HttpStream::read() << 16) | 
                    ((uint64_t)HttpStream::read() << 8)  |
                    (uint64_t)HttpStream::read(); 
    }

    // read in the mask, if present
    if (iRxMasked)
    {
        for (int i = 0; i < (int)sizeof(iRxMaskKey); i++)
        {
            iRxMaskKey[i] = HttpStream::read();
        }
    }

    iRxMaskIndex = 0;

    if (TYPE_CONNECTION_CLOSE == messageType())
    {
        flushRx();
        iRxSize = 0;
    }
    else if (TYPE_PING == messageType())
    {
        beginMessage(TYPE_PONG);
        while(available())
        {
            write(read());
        }
        endMessage();

        iRxSize = 0;
    }
    else if (TYPE_PONG == messageType())
    {
        flushRx();
        iRxSize = 0;
    }

    return iRxSize;
}

int WebSocketStream::messageType()
{
    return (iRxOpCode & 0x0f);
}

bool WebSocketStream::isFinal()
{
    return ((iRxOpCode & 0x80) != 0);
}

String WebSocketStream::readString()
{
    int avail = available();
    String s;

    if (avail > 0)
    {
        s.reserve(avail);

        for (int i = 0; i < avail; i++)
        {
            s += (char)read();
        }
    }

    return s;
}

int WebSocketStream::ping()
{
    uint8_t pingData[16];

    // create random data for the ping
    for (int i = 0; i < (int)sizeof(pingData); i++)
    {
        pingData[i] = random(0xff);
    }

    beginMessage(TYPE_PING);
    write(pingData, sizeof(pingData));
    return endMessage();
}

int WebSocketStream::available()
{
    if (iState < eReadingBody)
    {
        return HttpStream::available();
    }

    return iRxSize;
}

int WebSocketStream::read()
{
    byte b;

    if (read(&b, sizeof(b)))
    {
        return b;
    }

    return -1;
}

int WebSocketStream::read(uint8_t *aBuffer, size_t aSize)
{
    int readCount = HttpStream::readBytes(aBuffer, aSize);

    if (readCount > 0)
    {
        iRxSize -= readCount;

        // unmask the RX data if needed
        if (iRxMasked)
        {
            for (int i = 0; i < (int)aSize; i++, iRxMaskIndex++)
            {
                aBuffer[i] ^= iRxMaskKey[iRxMaskIndex % sizeof(iRxMaskKey)];
            }
        }
    }

    return readCount;
}

int WebSocketStream::peek()
{
    int p = HttpStream::peek();

    if (p != -1 && iRxMasked)
    {
        // unmask the RX data if needed
        p = (uint8_t)p ^ iRxMaskKey[iRxMaskIndex % sizeof(iRxMaskKey)];
    }

    return p;
}

void WebSocketStream::flushRx()
{
    while(available())
    {
        read();
    }
}
