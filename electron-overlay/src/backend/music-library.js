const fs = require("fs");
const path = require("path");
const { pathToFileURL } = require("url");
const { parseFile } = require("music-metadata");

function getAppDataDir() {
  return path.join(process.env.APPDATA || process.cwd(), "voidoverlay");
}

function getMusicDir() {
  const musicDir = path.join(getAppDataDir(), "music");
  fs.mkdirSync(musicDir, { recursive: true });
  return musicDir;
}

function toDataUrl(picture) {
  if (!picture?.data || !picture?.format) {
    return null;
  }
  return `data:${picture.format};base64,${picture.data.toString("base64")}`;
}

async function readTrack(filePath) {
  const fallbackTitle = path.basename(filePath, path.extname(filePath));
  try {
    const metadata = await parseFile(filePath, { duration: false, skipCovers: false });
    const title = metadata.common.title || fallbackTitle;
    const artist = metadata.common.artist || "";
    const album = metadata.common.album || "";
    return {
      id: filePath,
      path: filePath,
      url: pathToFileURL(filePath).href,
      title,
      artist,
      album,
      info: artist && title ? `${artist} - ${title}` : title,
      albumArtUrl: toDataUrl(metadata.common.picture?.[0])
    };
  } catch (_error) {
    return {
      id: filePath,
      path: filePath,
      url: pathToFileURL(filePath).href,
      title: fallbackTitle,
      artist: "",
      album: "",
      info: fallbackTitle,
      albumArtUrl: null
    };
  }
}

async function listTracks() {
  const musicDir = getMusicDir();
  const files = fs.readdirSync(musicDir)
    .filter((name) => name.toLowerCase().endsWith(".mp3"))
    .sort((left, right) => left.localeCompare(right))
    .map((name) => path.join(musicDir, name));

  return Promise.all(files.map((filePath) => readTrack(filePath)));
}

module.exports = {
  getMusicDir,
  listTracks
};
