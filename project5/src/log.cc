#include "../include/log.h"

std::list<log_t*> logBuffer;

int logFd = 0;
uint64_t lastLSN = 0;

int initLog() {
    if (openLogFile() == 0) {
        lastLSN = 0;
        return 0;
    } else {
        lastLSN = lseek(logFd, 0, SEEK_END);
        return 1;
    }

}

int openLogFile() {
    if ((logFd = open(LOG_FILE_NAME, O_RDWR | O_CREAT | O_EXCL , 0644)) > 0) {
        return 0;
    } else {
        logFd = open(LOG_FILE_NAME, O_RDWR);
        return 1;
    }
}

// for BEGIN, COMMIT, ABORT
int addLog(int transactionId, logType type) {
    log_t* newLog;
    newLog = (log_t*)malloc(sizeof(struct log_t));
    memset(newLog, 0, sizeof(log_t));
    if (logFd == 0) {
        initLog();
    }
    if (lastLSN == 0) {
        //first log
        newLog -> LSN = 40;
        newLog -> prevLSN = UINT64_MAX;
    } else {
        newLog -> LSN = lastLSN + 40;
        newLog -> prevLSN = lastLSN;
    }
    lastLSN = newLog -> LSN;
    newLog -> transactionId = transactionId;
    newLog -> type = type;
    logBuffer.push_back(newLog);
    return 0;
}

// for UPDATE
// return lsn of log to update pageLSN.
uint64_t addLog(int transactionId, logType type, int tableId, uint32_t pageNum, uint32_t offset, uint32_t dataLen, char* oldImage, char* newImage) {
    log_t* newLog;
    newLog = (log_t*)malloc(sizeof(struct log_t));
    memset(newLog, 0, sizeof(log_t));
    if (logFd == 0) {
        initLog();
    }
    if (lastLSN == 0) {
        //first log
        newLog -> LSN =  280;
        newLog -> prevLSN = UINT64_MAX;
    } else {
        newLog -> LSN = lastLSN + 280;
        newLog -> prevLSN = lastLSN;
    }
    lastLSN = newLog -> LSN;
    newLog -> transactionId = transactionId;
    newLog -> type = type;
    newLog -> tableId = tableId;
    newLog -> pageNum = pageNum;
    newLog -> offset = offset;
    newLog -> dataLen = dataLen;
    strcpy(newLog -> oldImage, oldImage);
    strcpy(newLog -> newImage, newImage);
    logBuffer.push_back(newLog);
    return newLog -> LSN;

}

int readLog(uint64_t prevLSN, log_t* dest) {
    lseek(logFd, prevLSN, SEEK_SET);
    read(logFd, dest, 40);

    if (dest -> type == UPDATE) {
        lseek(logFd, prevLSN, SEEK_SET);
        read(logFd, dest, 280);
    }
    return 0;
}

int flushLogBuffer() {
    if (logFd == 0) {
        // try to flush when log buffer is empty
        return -1;
    }
    log_t* log;

    while (!logBuffer.empty()) {
        log = logBuffer.front();
        if (log -> prevLSN == UINT64_MAX) {
            lseek(logFd, 0, SEEK_SET);
        } else {
            lseek(logFd, log -> prevLSN, SEEK_SET);
        }

        if (log -> type == UPDATE) {
            write(logFd, log, sizeof(struct log_t));
        } else {
            write(logFd, log, 40);
        }
        logBuffer.pop_front();
        free(log);
    }

    return 0;
}

int recovery() {
    if (logFd == 0) {
        initLog();
    }
    // map transactionId : last prevLSN
    std::map<int, std::list<log_t> > loser;
    analysisPass(loser);
    redoPass();
    undoPass(loser);
}

int analysisPass(std::map<int, std::list<log_t> >& loser) {
    // determine losers
    uint64_t prevLSN;
    log_t* log;

    log = (log_t*)malloc(sizeof(struct log_t));

    prevLSN = 0;
    while (prevLSN != lastLSN) {
        readLog(prevLSN, log);
        if (loser.find(log -> transactionId) != loser.end()) {
            if (log -> type == UPDATE) {
                for (auto it = loser[log -> transactionId].begin(); it != loser[log -> transactionId].end(); it++) {
                    if (it -> type == log -> type &&
                        it -> tableId == log -> tableId &&
                        it -> pageNum == log -> pageNum &&
                        it -> offset == log -> offset) {
                        // there already log exists on the same record.
                        // since we assume there's no double write in this project
                        // this is compensation log
                        loser[log -> transactionId].erase(it);
                        break; 
                    }
                }
            }
        }
        loser[log -> transactionId].push_back(*log);

        if (log -> type == COMMIT) {
            loser.erase(log -> transactionId);
        }
        prevLSN = log -> LSN;
    }

    free(log);
    return 0;
    
}
int redoPass() {
    // redo all log

    uint64_t prevLSN;
    log_t* log;
    pagenum_t pageNum;
    page_t* page;

    log = (log_t*)malloc(sizeof(struct log_t));
    prevLSN = 0;

    while (prevLSN != lastLSN) {
        readLog(prevLSN, log);
        if (log -> type != UPDATE) {
            prevLSN = log -> LSN;
            continue;
        }
        
        page = bufferRequestPage(log -> tableId, log -> pageNum);

        if (((leafPage_t*)page) -> pageLSN  < log -> LSN) {
            strcpy(((leafPage_t*)page) -> record[(log -> offset)/128 - 1].value, log -> newImage);
            bufferMakeDirty(log -> tableId, log -> pageNum);
        } 
        bufferUnpinPage(log -> tableId, log -> pageNum);
        prevLSN = log -> LSN;
    }

    free(log);
    return 0;
}
int undoPass(std::map<int, std::list<log_t> >& loser) {

    uint64_t prevLSN;
    uint64_t newLogLSN;
    log_t* log;
    pagenum_t pageNum;
    page_t* page;
    
    log = (log_t*)malloc(sizeof(struct log_t));

    int nextTransactionId;
    uint64_t nextEntry;

    while (!loser.empty()) {
        nextTransactionId = loser.begin() -> first;

        for (auto it = loser.begin(); it != loser.end(); it++) {
            if (loser[nextTransactionId].back().LSN < (it -> second).back().LSN) {
                nextTransactionId = it -> first;
            }
        }
        nextEntry = loser[nextTransactionId].back().prevLSN;


        readLog(nextEntry, log);
        if (log -> type == UPDATE) {

            page = bufferRequestPage(log -> tableId, log -> pageNum);

            if (((leafPage_t*)page) -> pageLSN  < log -> LSN) {
                newLogLSN = addLog(log -> transactionId, UPDATE, log -> tableId, log -> pageNum, log -> offset, log -> dataLen, log -> newImage,  log -> oldImage);
                strcpy(((leafPage_t*)page) -> record[(log -> offset)/128 - 1].value, log -> oldImage);
                page -> pageLSN = newLogLSN;
                bufferMakeDirty(log -> tableId, log -> pageNum);
                bufferUnpinPage(log -> tableId, log -> pageNum);
                
            }
            loser[log -> transactionId].pop_back();
        }

        if (log -> type == BEGIN) {
            loser.erase(log -> transactionId);
        }
    }
    
    
    return 0;
}