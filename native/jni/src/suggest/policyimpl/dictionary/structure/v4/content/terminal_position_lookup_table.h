/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LATINIME_TERMINAL_POSITION_LOOKUP_TABLE_H
#define LATINIME_TERMINAL_POSITION_LOOKUP_TABLE_H

#include <stdint.h>

#include "defines.h"
#include "suggest/policyimpl/dictionary/structure/v4/content/single_dict_content.h"
#include "suggest/policyimpl/dictionary/structure/v4/ver4_dict_constants.h"
#include "suggest/policyimpl/dictionary/structure/v4/ver4_patricia_trie_reading_utils.h"
#include "suggest/policyimpl/dictionary/utils/buffer_with_extendable_buffer.h"

namespace latinime {

class TerminalPositionLookupTable : public SingleDictContent {
 public:
    // TODO: Quit using headerRegionSize.
    TerminalPositionLookupTable(const char *const dictDirPath, const bool isUpdatable,
            const int headerRegionSize)
            : SingleDictContent(dictDirPath,
                      Ver4DictConstants::TERMINAL_ADDRESS_TABLE_FILE_EXTENSION, isUpdatable),
              mSize(getBuffer()->getTailPosition()
                      / Ver4DictConstants::TERMINAL_ADDRESS_TABLE_ADDRESS_SIZE),
              mHeaderRegionSize(headerRegionSize) {}

    TerminalPositionLookupTable() : mSize(0), mHeaderRegionSize(0) {}

    int getTerminalPtNodePosition(const int terminalId) const {
        if (terminalId < 0 || terminalId >= mSize) {
            return NOT_A_DICT_POS;
        }
        return getBuffer()->readUint(Ver4DictConstants::TERMINAL_ADDRESS_TABLE_ADDRESS_SIZE,
                getEntryPos(terminalId)) - mHeaderRegionSize;
    }

    bool setTerminalPtNodePosition(const int terminalId, const int terminalPtNodePos) {
        if (terminalId < 0) {
            return NOT_A_DICT_POS;
        }
        if (terminalId >= mSize) {
            int writingPos = getBuffer()->getTailPosition();
            while(writingPos <= getEntryPos(terminalId)) {
                // Write new entry.
                getWritableBuffer()->writeUintAndAdvancePosition(
                        Ver4DictConstants::NOT_A_TERMINAL_ADDRESS,
                        Ver4DictConstants::TERMINAL_ADDRESS_TABLE_ADDRESS_SIZE, &writingPos);
            }
            mSize = getBuffer()->getTailPosition()
                    / Ver4DictConstants::TERMINAL_ADDRESS_TABLE_ADDRESS_SIZE;
        }
        return getWritableBuffer()->writeUint(terminalPtNodePos + mHeaderRegionSize,
                Ver4DictConstants::TERMINAL_ADDRESS_TABLE_ADDRESS_SIZE, getEntryPos(terminalId));
    }

    int getNextTerminalId() const {
        return mSize;
    }

    bool flushToFile(const char *const dictDirPath, const int newHeaderRegionSize) const {
        const int headerRegionSizeDiff = newHeaderRegionSize - mHeaderRegionSize;
        // If header region size has been changed, terminal PtNode positions have to be adjusted
        // depending on the new header region size.
        if (headerRegionSizeDiff != 0) {
            TerminalPositionLookupTable lookupTableToWrite;
            for (int i = 0; i < mSize; ++i) {
                const int terminalPtNodePosition = getTerminalPtNodePosition(i)
                        + headerRegionSizeDiff;
                if (!lookupTableToWrite.setTerminalPtNodePosition(i, terminalPtNodePosition)) {
                    AKLOGE("Cannot set terminal position to lookupTableToWrite."
                            " terminalId: %d, position: %d", i, terminalPtNodePosition);
                    return false;
                }
            }
            return lookupTableToWrite.flush(dictDirPath,
                    Ver4DictConstants::TERMINAL_ADDRESS_TABLE_FILE_EXTENSION);
        } else {
            // We can simply use this lookup table because the header region size has not been
            // changed.
            return flush(dictDirPath, Ver4DictConstants::TERMINAL_ADDRESS_TABLE_FILE_EXTENSION);
        }
    }

 private:
    DISALLOW_COPY_AND_ASSIGN(TerminalPositionLookupTable);

    int getEntryPos(const int terminalId) const {
        return terminalId * Ver4DictConstants::TERMINAL_ADDRESS_TABLE_ADDRESS_SIZE;
    }

    int mSize;
    const int mHeaderRegionSize;
};
} // namespace latinime
#endif // LATINIME_TERMINAL_POSITION_LOOKUP_TABLE_H