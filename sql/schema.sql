-- S57-PostGIS Database Schema
-- Matching Njord's up.sql schema exactly

CREATE TABLE IF NOT EXISTS meta (
    key     VARCHAR UNIQUE NOT NULL,
    value   VARCHAR NULL
);

INSERT INTO meta VALUES ('version', '1') ON CONFLICT (key) DO NOTHING;

CREATE TABLE IF NOT EXISTS charts (
    id         BIGSERIAL PRIMARY KEY,
    name       VARCHAR UNIQUE           NOT NULL,
    scale      INTEGER                  NOT NULL,
    file_name  VARCHAR                  NOT NULL,
    updated    VARCHAR                  NOT NULL,
    issued     VARCHAR                  NOT NULL,
    zoom       INTEGER                  NOT NULL,
    covr       GEOMETRY(GEOMETRY, 4326) NOT NULL,
    dsid_props JSONB                    NOT NULL,
    chart_txt  JSONB                    NOT NULL
);

CREATE INDEX IF NOT EXISTS charts_gist ON charts USING GIST (covr);
CREATE INDEX IF NOT EXISTS charts_idx ON charts (id);

CREATE TABLE IF NOT EXISTS features (
    id        BIGSERIAL PRIMARY KEY,
    layer     VARCHAR                       NOT NULL,
    geom      GEOMETRY(GEOMETRY, 4326)      NOT NULL,
    props     JSONB                         NOT NULL,
    chart_id  BIGINT REFERENCES charts (id) NOT NULL,
    lnam_refs VARCHAR[]                     NULL,
    z_range   INT4RANGE                     NOT NULL
);

CREATE INDEX IF NOT EXISTS features_gist ON features USING GIST (geom);
CREATE INDEX IF NOT EXISTS features_idx ON features (id);
CREATE INDEX IF NOT EXISTS features_layer_idx ON features (layer);
CREATE INDEX IF NOT EXISTS features_zoom_idx ON features USING GIST (z_range);
CREATE INDEX IF NOT EXISTS features_lnam_idx ON features USING GIN (lnam_refs);
