-- Создание таблицы news для хранения новостей
CREATE TABLE IF NOT EXISTS news (
    id SERIAL PRIMARY KEY,
    title VARCHAR(500) NOT NULL,
    text TEXT NOT NULL,
    source VARCHAR(200) NOT NULL,
    date TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);

-- Создание индекса для быстрого поиска по дате
CREATE INDEX IF NOT EXISTS idx_news_date ON news(date DESC);

-- Создание индекса для поиска по источнику
CREATE INDEX IF NOT EXISTS idx_news_source ON news(source);

-- Вставка тестовых данных
INSERT INTO news (title, text, source, date) VALUES 
('Тестовая новость 1', 'Это первая тестовая новость для проверки работы сервиса.', 'test_source', NOW() - INTERVAL '1 hour'),
('Тестовая новость 2', 'Это вторая тестовая новость для проверки работы сервиса.', 'test_source', NOW() - INTERVAL '30 minutes'),
('Тестовая новость 3', 'Это третья тестовая новость для проверки работы сервиса.', 'test_source', NOW() - INTERVAL '10 minutes');
