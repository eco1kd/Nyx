# Organization backup and recovery

The original source/release snapshot is identified by diagnostics/latest-project-organization-backup.txt. The backup contains:
- sources-and-release.tar.gz: original JNI/vendor sources, resource licenses, active host tests, scripts/docs and previous out/ library.
- before-manifest.json: pre-migration paths, sizes and SHA-256 hashes.
- moves.json: all original-to-new renames, including chained renames.

Dumps, old logs/screenshots, older backup folders and generated caches were moved without deleting their bytes. They were not duplicated into the source tarball. The verification report is diagnostics/organization/preservation.json.

For inspection, extract sources-and-release.tar.gz into a NEW, separate directory, not over the organized project. Its internal paths follow the original layout. The original dump/logs/caches still exist at their destinations in moves.json.

For a full reversal, first save any subsequent work, then reverse moves.json in reverse order and restore source/config/script files from the tarball. Root compatibility wrappers/symlinks and newly created documentation require explicit review; do not overwrite or remove them blindly. No automatic destructive rollback was run or installed. Restoring after future development is a separate operation and should be confirmed.

The reorganized library may have a different hash/BuildID because compiler source paths changed. The original library's SHA-256 is verified against the tarball. Runtime code bodies and game offsets were preserved, but neither builds nor host tests establish live Aim functionality.
