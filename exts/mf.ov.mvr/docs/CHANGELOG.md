# Changelog

# [1.0.1] - 2024-10-18
- Fixed Null check issue for <ChildList>

# [1.0.0] - 2024-01-24
- Added native OpenUSD file format plugin for payload support.
- Fixed orientation and scale issues
- Some light parameters are now applied to USD light (cone, color temp, intensity)
- Deprecated kit 104 and 105.0
- Added Sample files for USDView

# [0.4.0] - 2023-10-02

# Added
- Sample file

# Fixed
- Enabled importing from Omniverse
- Importing within the same repository as the source file fixed (for filesystem and Omniverse)

# Changed
- The name of the folder (the one created during importation that contains the files converted to usd) won't include the file extension ("myMVRFile.mvr/" will now be "myMVRFile_mvr/")
- GDTF attributes populated by MVR now better reflect naming convention of the specs ("fixture_id" becomes "FixtureID")
- Properly remove the temporary directory created for archive extraction at the end of importation

# [0.3.0] - 2023-09-01

## Fixed
- Global scale rework
- Fix relative link issue with character escaping

# [0.2.0] - 2023-08-17

### Added
- Support for multiple layers
- Layers reflected as Scope in usd

### Changed
- When making name valid for usd, add underscore if starts with number

# [0.1.0] - 2023-07-21

### Added
- Initial version of the extension
- Support import of MVR files
