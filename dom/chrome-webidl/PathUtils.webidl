/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * PathUtils is a set of utilities for operating on absolute paths.
 */
[ChromeOnly, Exposed=(Window, Worker)]
namespace PathUtils {
  /**
   * Return the last path component.
   *
   * @param path An absolute path.
   *
   * @returns The last path component.
   */
  [Throws]
  DOMString filename(DOMString path);

  /**
   * Return the parent directory name of the given path.
   *
   * @param path An absolute path.
   *
   * @return The parent directory.
   *
   *         If the path provided is a root path (e.g., `C:` on Windows or `/`
   *         on *NIX), then null is returned.
   */
  [Throws]
  DOMString? parent(DOMString path);

  /**
   * Join the given components into a full path.
   *
   * @param components The path components. The first component must be an
   *                   absolute path.
   */
  [Throws]
  DOMString join(DOMString... components);

  /**
   * Normalize a path by removing multiple separators and `..` and `.`
   * directories.
   *
   * On UNIX platforms, the path must exist as symbolic links will be resolved.
   *
   * @param path The absolute path to normalize.
   *
   */
  [Throws]
  DOMString normalize(DOMString path);

  /**
   * Split a path into its components.
   *
   * @param path An absolute path.
   */
  [Throws]
  sequence<DOMString> split(DOMString path);

  /**
   * Transform a file path into a file: URI
   *
   * @param path An absolute path.
   *
   * @return The file: URI as a string.
   */
  [Throws]
  UTF8String toFileURI(DOMString path);
};
