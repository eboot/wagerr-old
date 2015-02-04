0.11.1 Release notes
====================

Darkcoin Core 0.11.1 supports a full implementation of InstantX, Darksend improvements
and a new version of enforcement compatible with the newer Bitcoin architechure.

- Fully implemented IX
- Added support for DSTX messages, as a result DS should be much faster
- Clear vValue in SelectCoinsMinConf - should fix an issue with conflicted txes
- "Debug window" -> "Tools window" renaming
- "Last Darksend message" text added in overview page
- Many new languages are supported, such as German, Vietnamese, Spanish
- Fixed required maturity of coins before sending
- New masternode payments enforcement implementation
- Added support to ignore IX confirmations when needed
- Added --instantxdepth, which will show X confirmations when a transaction lock is present
- fix coin control crash https://github.com/bitcoin/bitcoin/pull/5700
- always get only confirmed coins by AvailableCoins for every DS relative action
