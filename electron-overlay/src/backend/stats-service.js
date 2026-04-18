class StatsService {
  constructor() {
    this.dataCache = new Map();
    this.mcidCache = new Map();
    this.inFlight = new Map();
  }

  async lookupPlayer(mcid, { custom = false, nick = null } = {}) {
    const key = mcid.toLowerCase();
    if (this.inFlight.has(key)) {
      return this.inFlight.get(key);
    }

    const promise = this.lookupPlayerInternal(mcid, { custom, nick })
      .finally(() => this.inFlight.delete(key));
    this.inFlight.set(key, promise);
    return promise;
  }

  async lookupPlayerInternal(mcid, { custom = false, nick = null } = {}) {
    let mode = "name";
    let queryValue = mcid;
    const cached = this.dataCache.get(mcid.toLowerCase());
    if (cached && cached.lastUpdate > Date.now() - 60000) {
      return {
        status: "Success",
        data: this.parseBedwars(cached.payload, nick, custom)
      };
    }

    if (this.mcidCache.has(mcid.toLowerCase())) {
      mode = "uuid";
      queryValue = this.mcidCache.get(mcid.toLowerCase());
    }

    let response = null;
    for (let attempt = 0; attempt < 5; attempt += 1) {
      try {
        response = await fetch(`https://hypixel.voids.top/v2/player?${mode}=${encodeURIComponent(queryValue)}`, {
          headers: { "user-agent": "void-overlay-electron" }
        });
        break;
      } catch (_error) {
        if (attempt === 4) {
          return { status: "Failed", data: {} };
        }
      }
    }

    const data = await response.json();
    if (data.success && data.player) {
      this.dataCache.set(mcid.toLowerCase(), {
        lastUpdate: Date.now(),
        payload: data
      });
      return {
        status: "Success",
        data: this.parseBedwars(data, nick, custom)
      };
    }

    if (data.success && !data.player) {
      return { status: "Nicked", data: {} };
    }

    if (data.cause === "You have already looked up this name recently") {
      const resolved = await this.resolveUuidFromName(mcid);
      if (!resolved) {
        return { status: "Nicked", data: {} };
      }
      this.mcidCache.set(mcid.toLowerCase(), resolved);
      return this.lookupPlayerInternal(resolved, { custom, nick });
    }

    if (data.cause === "Key throttle") {
      return this.lookupPlayerInternal(mcid, { custom, nick });
    }

    return {
      status: data.cause || "Failed",
      data: {}
    };
  }

  async resolveUuidFromName(mcid) {
    try {
      const response = await fetch(`https://api.mojang.com/users/profiles/minecraft/${encodeURIComponent(mcid)}`);
      if (response.status === 204 || response.status === 429) {
        return null;
      }
      const data = await response.json();
      return data.id || null;
    } catch (_error) {
      return null;
    }
  }

  getRank(player) {
    if (player.rank) {
      return player.rank;
    }
    if (player.newPackageRank) {
      if (player.monthlyPackageRank && player.monthlyPackageRank !== "NONE") {
        return "MVP++";
      }
      return player.newPackageRank;
    }
    return "DEFAULT";
  }

  parseBedwars(payload, nick = null, custom = false) {
    const player = payload.player || {};
    const stats = player.stats?.Bedwars;
    const rank = this.getRank(player);

    if (!stats) {
      return {
        Player: player.displayname || nick || "Unknown",
        rank,
        TAG: "First Game",
        FKDR: "-",
        WLR: "-",
        Finals: "-",
        Wins: "-",
        WS: "-",
        star: 0,
        score: 10000
      };
    }

    const finalKills = Number(stats.final_kills_bedwars || 0);
    const finalDeaths = Number(stats.final_deaths_bedwars || 0);
    const totalWins = Number(stats.wins_bedwars || 0);
    const totalGames = Number(stats.games_played_bedwars || 0);
    const star = Number(player.achievements?.bedwars_level || 0);

    const fkdr = finalDeaths > 0 ? (finalKills / finalDeaths).toFixed(2) : finalKills.toFixed(2);
    const losses = Math.max(totalGames - totalWins, 0);
    const wlr = losses > 0 ? (totalWins / losses).toFixed(2) : totalWins.toFixed(2);
    const tags = [];
    if (finalDeaths === 0) {
      tags.push("no death");
    }
    if (custom) {
      tags.push("specified");
    }

    const name = nick ? `${player.displayname} (${nick})` : player.displayname;
    if (nick) {
      tags.push("nicked");
    }

    return {
      Player: name,
      rank,
      TAG: tags.join(","),
      FKDR: fkdr,
      WLR: wlr,
      Finals: finalKills,
      Wins: totalWins,
      WS: stats.winstreak ?? "-",
      star,
      score: Number(fkdr)
    };
  }
}

module.exports = {
  StatsService
};
